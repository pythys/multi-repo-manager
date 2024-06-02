#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <iostream>
#include "config.hpp"

RepoType to_repo_type(const std::string& str) {
    if (str == "git") return RepoType::GIT;
    if (str == "svn") return RepoType::SVN;
    throw std::runtime_error("Invalid RepoType: " + str);
}

Remote to_remote(const YAML::Node& node) {
    return {
        node["name"].as<std::string>(),
        node["url"].as<std::string>()
    };
}

Repo to_repo(const YAML::Node& node) {
    std::vector<Remote> remotes;
    if (node["remotes"] && node["remotes"].IsSequence()) {
        std::transform(
            node["remotes"].begin(),
            node["remotes"].end(),
            std::back_inserter(remotes),
            to_remote);
    } else {
        std::cerr << "Warning: 'remotes' node is missing or not a sequence "
                  << "for repo " << node["name"].as<std::string>("unknown")
                  << std::endl;
    }
    return {
        node["name"].as<std::string>(),
        to_repo_type(node["type"].as<std::string>()),
        RepoStatus::PENDING,
        remotes,
        std::vector<Repo>()
    };
}

Tree to_tree(const YAML::Node& node) {
    std::vector<Repo> repos;
    std::transform(
        node["repos"].begin(),
        node["repos"].end(),
        std::back_inserter(repos),
        to_repo);
    return {node["root"].as<std::string>(), repos};
}

bool is_direct_child(const Repo parent, const Repo child) {
    bool is_child_longer = child.name.size() > parent.name.size();
    bool contains_parent =
        child.name.substr(0, parent.name.size()) == parent.name;
    bool slash_separated = child.name[parent.name.size()] == '/';
    bool only_one_slash =
        child.name.find('/', parent.name.size() + 1) == std::string::npos;
    return is_child_longer && contains_parent &&
           slash_separated && only_one_slash;
}

std::vector<Repo> find_parents(
    const Repo repo,
    const std::vector<Repo> all_repos) {

    std::vector<Repo> parents;
    for (auto& parent : all_repos) {
        if (is_direct_child(parent, repo)) {
            parents.push_back(parent);
        }
    }
    return parents;
}

std::vector<Repo> find_children(
    const Repo repo,
    const std::vector<Repo> all_repos) {

    std::vector<Repo> children;
    for (auto& child : all_repos) {
        if (is_direct_child(repo, child)) {
            children.push_back(child);
        }
    }
    return children;
}

std::vector<Repo> to_dependency_tree(
    Repo parent_repo,
    std::vector<Repo> all_repos) {

    std::vector<Repo> child_repos = find_children(parent_repo, all_repos);
    if (!child_repos.empty()) {
        for (auto& child : child_repos) {
            child.children = to_dependency_tree(child, all_repos);
        }
    }
    return child_repos;
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
