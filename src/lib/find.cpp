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
    for (auto it = fs::recursive_directory_iterator(root);
         it != fs::end(it);
         ++it) {
        if (!it->is_directory()) {
            continue;
        }
        auto dirpath = it->path();
        auto filename = dirpath.filename().string();
        if (repo_map.find(filename) != repo_map.end()) {
            it.disable_recursion_pending();
        } else {
            auto repo_type_it = std::find_if(
                fs::directory_iterator(dirpath),
                fs::directory_iterator{},
                [repo_map](const auto& subentry) {
                    auto subname = subentry.path().filename().string();
                    return repo_map.find(subname) != repo_map.end();
                });

            if (repo_type_it != fs::directory_iterator{}) {
                auto repo_type_str = repo_type_it->path().filename().string();
                auto repo_type = repo_map.at(repo_type_str);
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
