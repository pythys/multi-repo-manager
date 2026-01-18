#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"

std::string from_repo_type(RepoType type) {
    switch (type) {
        case RepoType::GIT: return "git";
        case RepoType::SVN: return "svn";
        default: throw std::runtime_error("Unknown RepoType");
    }
}

RepoType to_repo_type(const std::string& str) {
    if (str == "git") {
        return RepoType::GIT;
    }
    if (str == "svn") {
        return RepoType::SVN;
    }
    throw std::runtime_error("Invalid RepoType: " + str);
}

Remote to_remote(const YAML::Node& node) {
    return Remote {
        .name = node["name"].as<std::string>(),
        .url = node["url"].as<std::string>()
    };
}

Repo to_repo(const YAML::Node& node) {
    std::vector<Remote> remotes;
    const auto remote_node = node["remotes"];
    if (remote_node.IsDefined() && remote_node.IsSequence()) {
        std::transform(
            node["remotes"].begin(),
            node["remotes"].end(),
            std::back_inserter(remotes),
            to_remote);
    } else {
        std::cerr << "Warning: 'remotes' node is missing or not a sequence "
                  << "for repo "
                  << node["name"].as<std::string>("unknown")
                  << '\n';
    }
    return Repo {
        .name = node["name"].as<std::string>(),
        .type = to_repo_type(node["type"].as<std::string>()),
        .status = RepoStatus::PENDING,
        .remotes = remotes,
        .children = {},
        .messages = {}
    };
}

Tree to_tree(const YAML::Node& node) {
    std::vector<Repo> repos;
    std::transform(
        node["repos"].begin(),
        node["repos"].end(),
        std::back_inserter(repos),
        to_repo);
    return {
        .root = node["root"].as<std::string>(),
        .repos = repos
    };
}

bool is_direct_child(
    const Repo parent,
    const Repo child,
    const std::vector<Repo>& all_repos) {
    const bool child_is_longer = child.name.size() > parent.name.size();
    const bool child_contains_parent = child.name.starts_with(parent.name + "/");
    const bool intermediate_exists = std::ranges::any_of(
        all_repos,
        [&](const Repo& middle) {
            bool middle_is_longer =
                middle.name.size() > parent.name.size();
            bool middle_is_shorter =
                middle.name.size() < child.name.size();
            bool middle_contains_parent =
                middle.name.starts_with(parent.name + "/");
            bool child_contains_middle =
                child.name.starts_with(middle.name + "/");
            return middle_is_longer &&
                middle_is_shorter &&
                middle_contains_parent &&
                child_contains_middle;
        });
    return child_is_longer && child_contains_parent && !intermediate_exists;
}

std::vector<Repo> find_parents(
    const Repo& repo,
    const std::vector<Repo>& all_repos) {

    std::vector<Repo> parents;
    for (const auto& parent : all_repos) {
        if (is_direct_child(parent, repo, all_repos)) {
            parents.push_back(parent);
        }
    }
    return parents;
}

std::vector<Repo> find_children(
    const Repo& repo,
    const std::vector<Repo>& all_repos) {

    std::vector<Repo> children;
    for (const auto& child : all_repos) {
        if (is_direct_child(repo, child, all_repos)) {
            children.push_back(child);
        }
    }
    return children;
}

std::vector<Repo> to_dependency_tree(
    Repo& parent_repo,
    std::vector<Repo>& all_repos) {

    std::vector<Repo> child_repos = find_children(parent_repo, all_repos);
    for (auto& child : child_repos) {
        child.children = to_dependency_tree(child, all_repos);
    }
    return child_repos;
}

std::string make_config(const std::vector<Tree>& trees) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "trees" << YAML::Value << YAML::BeginSeq;
    for (const auto& tree : trees) {
        out << YAML::BeginMap;
        out << YAML::Key << "root" << YAML::Value << tree.root;
        out << YAML::Key << "repos" << YAML::Value << YAML::BeginSeq;
        for (const auto& repo : tree.repos) {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << repo.name;
            out << YAML::Key
                << "type"
                << YAML::Value
                << from_repo_type(repo.type);
            out << YAML::Key << "remotes" << YAML::Value << YAML::BeginSeq;
            for (const auto& remote : repo.remotes) {
                out << YAML::BeginMap;
                out << YAML::Key << "name" << YAML::Value << remote.name;
                out << YAML::Key << "url" << YAML::Value << remote.url;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    return out.c_str();
}

void write_config(
    const std::vector<Tree>& trees,
    const std::string& config_file) {
    std::string config = make_config(trees);
    std::ofstream file(config_file);
    file << config;
    file.close();
}

std::vector<Tree> get_config(const std::string& config_file) {
    std::vector<Tree> trees;
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        if (config["trees"]) {
            std::transform(
                config["trees"].begin(),
                config["trees"].end(),
                std::back_inserter(trees),
                to_tree);
        } else {
            std::cerr << "Error: 'trees' node is missing."
                      << std::endl;
        }
    } catch (YAML::Exception &e) {
        std::cerr << "Error loading YAML file: "
                  << e.what()
                  << std::endl;
    }
    return trees;
}

std::vector<Tree> get_dependencies(const std::string& config_file) {
    std::vector<Tree> trees = get_config(config_file);
    for (auto& tree : trees) {
        std::vector<Repo> tree_repos;
        for (auto& repo : tree.repos) {
            std::vector<Repo> parent_repos = find_parents(repo, tree.repos);
            if (parent_repos.empty()) {
                repo.children = to_dependency_tree(repo, tree.repos);
                tree_repos.push_back(repo);
            }
        }
        tree.repos = tree_repos;
    }
    return trees;
}
