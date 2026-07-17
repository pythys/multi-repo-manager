#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include "util/constants.hpp"
#include "vcs/git_manager.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {
namespace fs = std::filesystem;

Remote to_remote(const YAML::Node &node) {
    return Remote{
        .name = node["name"].as<std::string>(),
        .url = node["url"].as<std::string>()};
}

Branch to_branch(const YAML::Node &node) {
    return Branch{
        .name = node["name"].as<std::string>(),
        .remote = node["remote"].as<std::string>(),
        .is_current = node["is_current"].as<bool>()};
}

Repo to_repo(const YAML::Node &node) {
    std::vector<Remote> remotes;
    std::vector<Branch> branches;
    const auto remote_node = node["remotes"];
    const auto branch_node = node["branches"];
    if (remote_node.IsDefined() && remote_node.IsSequence()) {
        std::ranges::transform(
            remote_node,
            std::back_inserter(remotes),
            to_remote);
        if (branch_node.IsDefined() && branch_node.IsSequence()) {
            std::ranges::transform(
                branch_node,
                std::back_inserter(branches),
                to_branch);
        }
    } else {
        std::cerr << "Warning: 'remotes' node is missing or not a sequence "
                  << "for repo " << node["name"].as<std::string>("unknown")
                  << '\n';
    }
    return Repo{
        .name = node["name"].as<std::string>(),
        .phase = RepoPhase::QUEUED,
        .remotes = remotes,
        .branches = branches,
        .children = {},
        .messages = {}};
}

Tree to_tree(const YAML::Node &node) {
    std::vector<Repo> repos;
    std::ranges::transform(node["repos"], std::back_inserter(repos), to_repo);
    return {.root = node["root"].as<std::string>(), .repos = repos};
}

std::vector<Tree> parse_config_text(const std::string &yaml) {
    std::vector<Tree> trees;
    try {
        YAML::Node config = YAML::Load(yaml);
        if (config["trees"]) {
            std::ranges::transform(
                config["trees"],
                std::back_inserter(trees),
                to_tree);
        } else {
            std::cerr << "Error: 'trees' node is missing.\n";
        }
    } catch (YAML::Exception &e) {
        std::cerr << "Error loading YAML file: " << e.what() << '\n';
    }
    return trees;
}

struct RemoteConfigSpec {
    std::string repo_url;
    std::string path;
    std::string ref;
};

std::optional<RemoteConfigSpec> parse_remote_config(const std::string &value) {
    std::size_t search_start = 0;
    const std::size_t scheme = value.find("://");
    if (scheme != std::string::npos) {
        search_start = scheme + 3;
    }

    const std::size_t separator = value.find("//", search_start);
    if (separator == std::string::npos) {
        return std::nullopt;
    }

    RemoteConfigSpec spec;
    spec.repo_url = value.substr(0, separator);
    std::string remainder = value.substr(separator + 2);

    const std::size_t query = remainder.find('?');
    if (query != std::string::npos) {
        const std::string params = remainder.substr(query + 1);
        remainder = remainder.substr(0, query);
        const std::string ref_key = "ref=";
        if (params.starts_with(ref_key)) {
            spec.ref = params.substr(ref_key.size());
        }
    }
    spec.path = remainder;

    if (spec.repo_url.empty() || spec.path.empty()) {
        return std::nullopt;
    }
    return spec;
}

bool is_safe_path(const std::string &path) {
    const fs::path candidate(path);
    if (candidate.is_absolute()) {
        return false;
    }
    return std::ranges::none_of(candidate, [](const fs::path &part) {
        return part == "..";
    });
}

class TempCloneDir {
  public:
    TempCloneDir() {
        const auto unique = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = fs::temp_directory_path() / ("mrm-config-" + unique);
    }
    ~TempCloneDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    const fs::path &path() const {
        return path_;
    }
    TempCloneDir(const TempCloneDir &) = delete;
    TempCloneDir &operator=(const TempCloneDir &) = delete;

  private:
    fs::path path_;
};

std::string fetch_remote_config(const RemoteConfigSpec &spec) {
    if (!is_safe_path(spec.path)) {
        throw std::runtime_error(
            "Invalid config path in remote config: " + spec.path);
    }

    const TempCloneDir clone_dir;
    const std::string destination = clone_dir.path().string();
    try {
        GitManager::clone(
            spec.repo_url,
            destination,
            DEFAULT_TIMEOUT,
            1,
            spec.ref);
    } catch (const std::exception &) {
        std::error_code ec;
        fs::remove_all(clone_dir.path(), ec);
        GitManager::clone(
            spec.repo_url,
            destination,
            DEFAULT_TIMEOUT,
            0,
            spec.ref);
    }

    const fs::path config_path = clone_dir.path() / spec.path;
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Config file '" + spec.path + "' not found in " + spec.repo_url);
    }
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
}

std::string child_prefix(const std::string &name) {
    return name == "." ? std::string{} : name + "/";
}

bool is_direct_child(
    const Repo &parent,
    const Repo &child,
    const std::vector<Repo> &all_repos) {
    if (child.name == parent.name) {
        return false;
    }
    const std::string parent_prefix = child_prefix(parent.name);
    if (!child.name.starts_with(parent_prefix)) {
        return false;
    }
    const bool intermediate_exists =
        std::ranges::any_of(all_repos, [&](const Repo &middle) {
            if (middle.name == parent.name || middle.name == child.name) {
                return false;
            }
            const bool middle_contains_parent =
                middle.name.starts_with(parent_prefix);
            const bool child_contains_middle =
                child.name.starts_with(child_prefix(middle.name));
            return middle_contains_parent && child_contains_middle;
        });
    return !intermediate_exists;
}

std::vector<Repo>
find_parents(const Repo &repo, const std::vector<Repo> &all_repos) {
    std::vector<Repo> parents;
    for (const auto &parent : all_repos) {
        if (is_direct_child(parent, repo, all_repos)) {
            parents.push_back(parent);
        }
    }
    return parents;
}

std::vector<Repo>
find_children(const Repo &repo, const std::vector<Repo> &all_repos) {
    std::vector<Repo> children;
    for (const auto &child : all_repos) {
        if (is_direct_child(repo, child, all_repos)) {
            children.push_back(child);
        }
    }
    return children;
}

std::vector<Repo> to_dependency_tree(
    const Repo &parent_repo,
    const std::vector<Repo> &all_repos) {
    std::vector<Repo> child_repos = find_children(parent_repo, all_repos);
    for (auto &child : child_repos) {
        child.children = to_dependency_tree(child, all_repos);
    }
    return child_repos;
}

bool root_matches_pattern(const std::string &root, const std::string &pattern) {
    std::size_t root_index = 0;
    std::size_t pattern_index = 0;
    std::size_t star_index = std::string::npos;
    std::size_t match_index = 0;

    while (root_index < root.size()) {
        const bool char_matches =
            pattern_index < pattern.size() &&
            (pattern.at(pattern_index) == '?' ||
             pattern.at(pattern_index) == root.at(root_index));
        if (char_matches) {
            ++pattern_index;
            ++root_index;
            continue;
        }
        const bool is_star =
            pattern_index < pattern.size() && pattern.at(pattern_index) == '*';
        if (is_star) {
            star_index = pattern_index++;
            match_index = root_index;
            continue;
        }
        if (star_index == std::string::npos) {
            return false;
        }
        pattern_index = star_index + 1;
        root_index = ++match_index;
    }

    while (pattern_index < pattern.size() && pattern.at(pattern_index) == '*') {
        ++pattern_index;
    }
    return pattern_index == pattern.size();
}
} // namespace

std::string make_config(const std::vector<Tree> &trees) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "trees" << YAML::Value << YAML::BeginSeq;
    for (const auto &tree : trees) {
        out << YAML::BeginMap;
        out << YAML::Key << "root" << YAML::Value << tree.root;
        out << YAML::Key << "repos" << YAML::Value << YAML::BeginSeq;
        for (const auto &repo : tree.repos) {
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << repo.name;
            out << YAML::Key << "remotes" << YAML::Value << YAML::BeginSeq;
            for (const auto &remote : repo.remotes) {
                out << YAML::BeginMap;
                out << YAML::Key << "name" << YAML::Value << remote.name;
                out << YAML::Key << "url" << YAML::Value << remote.url;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
            out << YAML::Key << "branches" << YAML::Value << YAML::BeginSeq;
            for (const auto &branch : repo.branches) {
                out << YAML::BeginMap;
                out << YAML::Key << "name" << YAML::Value << branch.name;
                out << YAML::Key << "remote" << YAML::Value << branch.remote;
                out << YAML::Key << "is_current" << YAML::Value
                    << branch.is_current;
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
    const std::vector<Tree> &trees,
    const std::string &config_file) {
    const std::string config = make_config(trees);
    std::ofstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open file for writing: " + config_file);
    }
    file << config;
    if (file.fail()) {
        throw std::runtime_error("Failed to write to file: " + config_file);
    }
    file.close();
}

std::vector<Tree> get_config(const std::string &config_file) {
    if (const auto spec = parse_remote_config(config_file)) {
        return parse_config_text(fetch_remote_config(*spec));
    }

    std::error_code ec;
    if (!std::filesystem::exists(config_file, ec)) {
        throw std::runtime_error(
            "Config file not found: " + config_file +
            "\nUse --config <file>, --find <paths>, or run `mrm find "
            "--save` to create one.");
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_file);
    }
    std::string text;
    text.assign(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());
    return parse_config_text(text);
}

std::vector<Tree> get_dependencies(const std::string &config_file) {
    std::vector<Tree> trees = get_config(config_file);
    for (auto &tree : trees) {
        std::vector<Repo> tree_repos;
        for (auto &repo : tree.repos) {
            const std::vector<Repo> parent_repos =
                find_parents(repo, tree.repos);
            if (parent_repos.empty()) {
                repo.children = to_dependency_tree(repo, tree.repos);
                tree_repos.push_back(repo);
            }
        }
        tree.repos = tree_repos;
    }
    return trees;
}

std::vector<Tree> filter_trees_by_root(
    const std::vector<Tree> &trees,
    const std::vector<std::string> &root_patterns) {
    if (root_patterns.empty()) {
        return trees;
    }

    std::vector<Tree> filtered;
    for (const auto &tree : trees) {
        const bool matches =
            std::ranges::any_of(root_patterns, [&](const std::string &pattern) {
                return root_matches_pattern(tree.root, pattern);
            });
        if (matches) {
            filtered.push_back(tree);
        }
    }
    return filtered;
}

std::vector<Tree> filter_trees_by_name(
    const std::vector<Tree> &trees,
    const std::vector<std::string> &name_patterns) {
    if (name_patterns.empty()) {
        return trees;
    }

    std::vector<Tree> filtered;
    for (const auto &tree : trees) {
        Tree filtered_tree = tree;
        filtered_tree.repos.clear();

        for (const auto &repo : tree.repos) {
            const bool matches = std::ranges::any_of(
                name_patterns,
                [&](const std::string &pattern) {
                    return root_matches_pattern(repo.name, pattern);
                });
            if (matches) {
                filtered_tree.repos.push_back(repo);
            }
        }

        if (!filtered_tree.repos.empty()) {
            filtered.push_back(filtered_tree);
        }
    }

    return filtered;
}

std::vector<Tree> load_trees(
    const std::string &config_file,
    const std::vector<std::string> &find_paths,
    const std::vector<std::string> &root_patterns,
    const std::vector<std::string> &name_patterns,
    int min_depth) {
    std::vector<Tree> trees;

    if (!find_paths.empty()) {
        for (const auto &path : find_paths) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec) || ec) {
                throw std::runtime_error("Path not found: " + path);
            }
        }

        for (const auto &path : find_paths) {
            std::string normalized = normalize_path(path);
            trees.push_back(
                Tree{
                    .root = normalized,
                    .repos = find_repos(normalized, min_depth)});
        }
    } else {
        trees = get_config(config_file);
    }

    trees = filter_trees_by_root(trees, root_patterns);
    trees = filter_trees_by_name(trees, name_patterns);

    return trees;
}
