#include "persistence/discovery.hpp"
#include "core/tree.hpp"
#include "vcs/git_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

bool is_accessible_directory(const fs::path &path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool has_entries(const fs::path &path) {
    std::error_code ec;
    fs::directory_iterator it(
        path,
        fs::directory_options::skip_permission_denied,
        ec);
    return !ec && it != fs::directory_iterator{};
}

bool is_git_repo(const fs::path &dir) {
    const fs::path git_dir = dir / ".git";
    return is_accessible_directory(git_dir) && has_entries(git_dir);
}
} // namespace

std::string normalize_path(const std::string &path) {
    const fs::path fsp(path);
    const std::string normalized = fsp.lexically_normal().string();
    return normalized.starts_with("./") ? normalized.substr(2) : normalized;
}

std::vector<Repo> find_repos(const std::string &path) {
    std::vector<Repo> repos;
    const fs::path root(path);
    if (!is_accessible_directory(root)) {
        return repos;
    }

    using Walker = fs::recursive_directory_iterator;
    std::error_code ec;
    Walker it(root, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        return repos;
    }
    const Walker end;
    while (it != end) {
        if (ec || !it->is_directory(ec)) {
            ec.clear();
            it.increment(ec);
            continue;
        }

        const fs::path dirpath = it->path();
        const std::string filename = dirpath.filename().string();
        if (filename == ".git") {
            it.disable_recursion_pending();
            it.increment(ec);
            continue;
        }

        if (is_git_repo(dirpath)) {
            try {
                auto remotes = GitManager::get_remotes(dirpath);
                auto branches = GitManager::get_branches(dirpath);
                Repo repo;
                repo.name = fs::relative(dirpath, root).string();
                repo.remotes = remotes;
                repo.branches = branches;
                repos.push_back(repo);
            } catch (const std::exception &e) {
                std::cerr << "Skipping " << dirpath << ": " << e.what() << '\n';
            }
        }

        it.increment(ec);
    }

    std::ranges::sort(repos, [](const Repo &prev, const Repo &next) {
        return prev.name < next.name;
    });

    return repos;
}
