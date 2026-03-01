#include "find.hpp"
#include "config.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
using RepoMarker = std::pair<const char *, RepoType>;

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

std::optional<RepoType> find_repo_type(const fs::path &dir) {
    static constexpr std::array<RepoMarker, 2> kMarkers = {
        RepoMarker{".git", RepoType::GIT},
        RepoMarker{".svn", RepoType::SVN},
    };

    for (const auto &[marker, type] : kMarkers) {
        const fs::path marker_path = dir / marker;
        if (is_accessible_directory(marker_path) && has_entries(marker_path)) {
            return type;
        }
    }

    return std::nullopt;
}
} // namespace

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
        if (filename == ".git" || filename == ".svn") {
            it.disable_recursion_pending();
            it.increment(ec);
            continue;
        }

        const auto repo_type = find_repo_type(dirpath);
        if (repo_type.has_value()) {
            try {
                auto repo_manager = create_repo_manager(*repo_type);
                auto remotes = repo_manager->get_remotes(dirpath);
                auto branches = repo_manager->get_branches(dirpath);
                Repo repo;
                repo.name = fs::relative(dirpath, root).string();
                repo.type = *repo_type;
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

std::string normalize_path(const std::string &path) {
    const fs::path fsp(path);
    const std::string normalized = fsp.lexically_normal().string();
    return normalized.starts_with("./") ? normalized.substr(2) : normalized;
}

int run_find(
    const std::vector<std::string> &find_paths,
    const std::string &save_path) {
    std::vector<std::string> roots = find_paths;
    if (roots.empty()) {
        roots.emplace_back(".");
    }

    std::vector<Tree> trees;
    for (const auto &path : roots) {
        const std::string normalized_root = normalize_path(path);
        trees.push_back(
            Tree{
                .root = normalized_root,
                .repos = find_repos(normalized_root)});
    }

    std::string config_output = make_config(trees);
    if (!save_path.empty()) {
        std::ofstream file(save_path);
        if (config_output.empty() || config_output.back() != '\n') {
            config_output += '\n';
        }
        file << config_output;
        file.close();
        std::cout << "Config saved to " << save_path << "\n";
    } else {
        std::cout << config_output << "\n";
    }
    return 0;
}
