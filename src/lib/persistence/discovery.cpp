#include "persistence/discovery.hpp"
#include "core/tree.hpp"
#include "vcs/git_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
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
    std::string normalized = fsp.lexically_normal().string();
    if (normalized.starts_with("./")) {
        normalized = normalized.substr(2);
    }
    if (normalized.size() > 1 && normalized.back() == '/') {
        normalized.pop_back();
    }
    return normalized;
}

std::vector<Repo>
find_repos(const std::string &path, int min_depth, int max_depth) {
    std::vector<Repo> repos;
    std::map<std::string, int> repo_depths;
    const fs::path root(path);
    if (!is_accessible_directory(root)) {
        return repos;
    }

    const auto collect = [&](const fs::path &dirpath) {
        const std::string name = fs::relative(dirpath, root).string();
        int depth = 0;
        if (name != ".") {
            depth = 1;
            fs::path parent = dirpath.parent_path();
            while (true) {
                const std::string parent_name =
                    fs::relative(parent, root).string();
                if (parent_name == ".") {
                    const auto root_depth = repo_depths.find(".");
                    if (root_depth != repo_depths.end()) {
                        depth = root_depth->second + 1;
                    }
                    break;
                }
                const auto parent_depth = repo_depths.find(parent_name);
                if (parent_depth != repo_depths.end()) {
                    depth = parent_depth->second + 1;
                    break;
                }
                parent = parent.parent_path();
            }
        }

        repo_depths[name] = depth;
        if (depth < min_depth || depth > max_depth) {
            return depth >= max_depth;
        }

        try {
            Repo repo;
            repo.name = name;
            repo.remotes = GitManager::get_remotes(dirpath);
            repo.branches = GitManager::get_branches(dirpath);
            repos.push_back(repo);
        } catch (const std::exception &e) {
            std::cerr << "Skipping " << dirpath << ": " << e.what() << '\n';
        }
        return depth >= max_depth;
    };

    if (is_git_repo(root)) {
        collect(root);
    }

    if (max_depth == 0) {
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
        const bool is_symlink = it->is_symlink(ec);
        if (ec || is_symlink || !it->is_directory(ec)) {
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

        if (is_git_repo(dirpath) && collect(dirpath)) {
            it.disable_recursion_pending();
        }

        it.increment(ec);
    }

    std::ranges::sort(repos, [](const Repo &prev, const Repo &next) {
        return prev.name < next.name;
    });

    return repos;
}
