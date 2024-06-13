#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/asio.hpp>
#include "config.hpp"
#include "find.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;
using boost::asio::io_context;
using boost::asio::thread_pool;
using boost::asio::post;

Repo create_repo(
    const fs::path& dirpath,
    RepoType repo_type,
    const fs::path& root) {

    auto repo_manager = create_repo_manager(repo_type);
    auto remotes = repo_manager->get_remotes(dirpath);
    Repo repo;
    repo.name = fs::relative(dirpath, root).string();
    repo.type = repo_type;
    repo.remotes = remotes;
    return repo;
}

void process_directory(
    const fs::path& dir,
    const std::unordered_map<std::string, RepoType>& repo_map,
    const fs::path& root,
    std::vector<Repo>* repos) {

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_directory()) {
            continue;
        }
        auto dirpath = entry.path();
        auto filename = dirpath.filename().string();
        if (repo_map.find(filename) != repo_map.end()) {
            repos->push_back(create_repo(dirpath, repo_map.at(filename), root));
            continue;
        }
        auto repo_type_it = std::find_if(
            fs::directory_iterator(dirpath),
            fs::directory_iterator{},
            [repo_map](const auto& subentry) {
                auto subname = subentry.path().filename().string();
                return repo_map.find(subname) != repo_map.end();
            });
        if (repo_type_it != fs::directory_iterator{}) {
            Repo repo = create_repo(
                dirpath,
                repo_map.at(repo_type_it->path().filename().string()),
                root);
            repos->push_back(repo);
        }
    }
}

std::vector<Repo> find_repos(const std::string& path) {
    std::vector<Repo> repos;
    fs::path root(path);
    std::unordered_map<std::string, RepoType> repo_map = {
        {".git", RepoType::GIT},
        {".svn", RepoType::SVN}
    };
    if (!fs::exists(root) || !fs::is_directory(root)) {
        return repos;
    }
    io_context io_context;
    thread_pool pool(std::thread::hardware_concurrency());
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_directory()) {
            continue;
        }
        post(pool, [&repos, entry, &repo_map, &root, &io_context]() {
            std::vector<Repo> sub_repos;
            process_directory(entry.path(), repo_map, root, &sub_repos);
            post(io_context, [&repos, sub_repos]() {
                repos.insert(repos.end(), sub_repos.begin(), sub_repos.end());
            });
        });
    }
    pool.join();
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
