#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "config.hpp"
#include "find.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;

std::vector<Repo> find_repos(const std::string& path) {
    std::vector<Repo> repos;
    fs::path root(path);
    std::unordered_map<std::string, RepoType> repo_map = {
        {".git", RepoType::GIT},
        {".svn", RepoType::SVN}
    };
    const bool valid_root = fs::exists(root) && fs::is_directory(root);
    if (!valid_root) {
        return repos;
    }
    using walker = fs::recursive_directory_iterator;
    for (auto it = walker(root); it != fs::end(it); ++it) {
        if (!it->is_directory()) {
            continue;
        }
        auto dirpath = it->path();
        auto filename = dirpath.filename().string();
        if (repo_map.find(filename) != repo_map.end()) {
            it.disable_recursion_pending();
        } else {
            auto repo_type_it = std::find_if(
                repo_map.begin(),
                repo_map.end(),
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
    std::sort(
        repos.begin(),
        repos.end(),
        [](const Repo& a, const Repo& b) {
            return a.name < b.name;
        });

    return repos;
}

std::string normalize_path(const std::string& path) {
    fs::path p(path);
    std::string normalized = p.lexically_normal().string();
    return (normalized.compare(0, 2, "./") == 0)
        ? normalized.substr(2)
        : normalized;
}

int run_find(const std::string& path) {
    Tree tree;
    std::vector<Repo> repos = find_repos(path);
    fs::path root_path = normalize_path(path);
    tree.root = root_path;
    tree.repos = repos;
    std::vector<Tree> trees = {tree};
    std::cout << make_config(trees) << std::endl;
    return 0;
}
