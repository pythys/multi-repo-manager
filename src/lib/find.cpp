#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "find.hpp"
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
                Repo repo;
                repo.name = filename;
                repo.type = repo_type;
                repos.push_back(repo);
            }
        }
    }
    return repos;
}

int run_find(const std::string& path) {
    std::vector<Repo> repos = find_repos(path);
    for (const auto& repo : repos) {
        std::cout << repo.name << std::endl;
    }
    return 0;
}
