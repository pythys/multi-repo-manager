#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "config.hpp"
#include "find.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;

namespace {
auto find_repos(const std::string& path) -> std::vector<Repo> {
    std::vector<Repo> repos;
    const fs::path root(path);
    std::unordered_map<std::string, RepoType> repo_map = {
        {".git", RepoType::GIT},
        {".svn", RepoType::SVN}
    };
    const bool valid_root = fs::exists(root) && fs::is_directory(root);
    if (!valid_root) {
        return repos;
    }
    using walker = fs::recursive_directory_iterator;
    for (auto it = walker(root);
         it != fs::end(it);  // NOLINT(misc-include-cleaner)
         ++it) {
        if (!it->is_directory()) {
            continue;
        }
        auto dirpath = it->path();
        auto filename = dirpath.filename().string();
        if (repo_map.find(filename) != repo_map.end()) {
            it.disable_recursion_pending();
        } else {
            auto repo_type_it = std::ranges::find_if(
                repo_map,
                [&dirpath](const auto& pair) {
                    return fs::exists(dirpath / pair.first);
                });

            if (repo_type_it != repo_map.end()) {
                auto repo_type = repo_type_it->second;
                auto repo_manager = create_repo_manager(repo_type);
                auto remotes = repo_manager->get_remotes(dirpath);
                Repo repo;
                repo.name = fs::relative(dirpath, root).string();
                repo.type = repo_type;
                repo.remotes = remotes;
                repos.push_back(repo);
            }
        }
    }
    std::ranges::sort(
        repos,
        [](const Repo& prev, const Repo& next) {
            return prev.name < next.name;
        });

    return repos;
}

auto normalize_path(const std::string& path) -> std::string {
    const fs::path fsp(path);
    const std::string normalized = fsp.lexically_normal().string();
    return normalized.starts_with("./")
        ? normalized.substr(2)
        : normalized;
}
}  // namespace

auto run_find(
    const std::string& find_path,
    const std::string& save_path) -> int {
    Tree tree;
    std::vector<Repo> repos = find_repos(find_path);
    fs::path root_path = normalize_path(find_path);
    tree.root = root_path;
    tree.repos = repos;
    std::vector<Tree> trees = {tree};
    std::string config_output = make_config(trees);
    if (!save_path.empty()) {
        std::ofstream file(save_path);
        if (config_output.empty() || config_output.back() != '\n') {
            config_output += '\n';
        }
        file << config_output;
        file.close();
        std::cout << "Config saved to " << save_path << std::endl;
    } else {
        std::cout << config_output << std::endl;
    }
    return 0;
}
