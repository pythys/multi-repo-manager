#include "sync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include "utils.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace asio = boost::asio;

namespace {
enum class MatchType : std::uint8_t { TO_REMOVE, TO_ADD };

template <typename T, typename CompareFunc>
std::vector<T> find_differences(
    const std::vector<T> &source,
    const std::vector<T> &target,
    MatchType match_type,
    CompareFunc compare) {

    std::vector<T> result;

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto &target_item : target) {
            auto it = std::ranges::find_if(source, [&](const T &source_item) {
                return compare(source_item, target_item);
            });
            if (it == source.end()) {
                result.push_back(target_item);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto &source_item : source) {
            auto it = std::ranges::find_if(target, [&](const T &target_item) {
                return compare(target_item, source_item);
            });
            if (it == target.end()) {
                result.push_back(source_item);
            }
        }
    }
    return result;
}

std::vector<Remote> find_remotes(
    const std::vector<Remote> &conf_remotes,
    const std::vector<Remote> &repo_remotes,
    MatchType match_type) {
    auto compare_by_name = [](const Remote &a, const Remote &b) {
        return a.name == b.name;
    };
    return find_differences(
        conf_remotes,
        repo_remotes,
        match_type,
        compare_by_name);
}

std::vector<Branch> find_branches(
    const std::vector<Branch> &conf_branches,
    const std::vector<Branch> &repo_branches,
    MatchType match_type) {
    auto compare_by_name = [](const Branch &a, const Branch &b) {
        return a.name == b.name;
    };
    return find_differences(
        conf_branches,
        repo_branches,
        match_type,
        compare_by_name);
}

void sync_branches(
    Tracker &tracker,
    const std::string &root,
    const std::string &repo_name,
    RepoManager *repo_manager,
    const std::string &repo_path,
    const std::vector<Branch> &desired_branches,
    bool prune_branches) {
    if (desired_branches.empty()) {
        return;
    }

    auto repo_branches = repo_manager->get_branches(repo_path);
    const Branch *original = find_current_branch(repo_branches);
    const std::string original_branch = original ? original->name : "";
    auto to_add_branches =
        find_branches(desired_branches, repo_branches, MatchType::TO_ADD);
    for (const auto &branch : to_add_branches) {
        if (branch.remote.empty()) {
            throw std::runtime_error(
                "Branch remote is missing for " + branch.name);
        }
        if (!repo_manager->branch_exists(repo_path, branch.name)) {
            repo_manager->pull_branch(
                repo_path,
                branch.remote,
                branch.name,
                branch.name);
        }
        repo_manager->switch_branch(repo_path, branch.name);
        repo_manager
            ->pull_branch(repo_path, branch.remote, branch.name, branch.name);
    }
    auto to_remove_branches =
        find_branches(desired_branches, repo_branches, MatchType::TO_REMOVE);
    if (prune_branches) {
        for (const auto &branch : to_remove_branches) {
            repo_manager->remove_branch(repo_path, branch);
        }
    } else {
        for (const auto &branch : to_remove_branches) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "Ignoring branch not in config: " + branch.name,
                MessageLevel::WARNING);
        }
    }
    if (!original_branch.empty()) {
        repo_manager->switch_branch(repo_path, original_branch);
    }
}
} // namespace

void update_repository(
    const std::string &root,
    Repo *repo,
    Tracker &tracker,
    bool prune_remotes,
    bool prune_branches) {
    tracker
        .set_phase(root, repo->name, RepoPhase::RUNNING, "Updating repository");
    auto repo_manager = create_repo_manager(repo->type);
    const std::string repo_path =
        (std::filesystem::path(root) / repo->name).string();
    auto remotes = repo_manager->get_remotes(repo_path);
    if (prune_remotes) {
        auto to_remove =
            find_remotes(repo->remotes, remotes, MatchType::TO_REMOVE);
        for (const auto &remote : to_remove) {
            repo_manager->remove_remote(repo_path, remote);
        }
    }
    auto to_add = find_remotes(repo->remotes, remotes, MatchType::TO_ADD);
    for (const auto &remote : to_add) {
        repo_manager->add_remote(repo_path, remote);
    }
    sync_branches(
        tracker,
        root,
        repo->name,
        repo_manager.get(),
        repo_path,
        repo->branches,
        prune_branches);
}

void clone_repository(const std::string &root, Repo *repo, Tracker &tracker) {
    tracker
        .set_phase(root, repo->name, RepoPhase::RUNNING, "Cloning repository");
    auto repo_manager = create_repo_manager(repo->type);
    auto it = std::ranges::find_if(repo->remotes, [](const Remote &remote) {
        return remote.name == "origin";
    });
    if (it != repo->remotes.end()) {
        repo_manager->copy(
            it->url,
            (std::filesystem::path(root) / repo->name).string());
    } else {
        throw std::runtime_error(
            "No remote found with name 'origin' for repo: " + repo->name);
    }
    for (const auto &remote : repo->remotes) {
        if (remote.name == "origin") {
            continue;
        }
        repo_manager->add_remote(
            (std::filesystem::path(root) / repo->name).string(),
            remote);
    }
    sync_branches(
        tracker,
        root,
        repo->name,
        repo_manager.get(),
        (std::filesystem::path(root) / repo->name).string(),
        repo->branches,
        false);
}

void sync_repository(
    const std::string &root,
    Repo *repo,
    Tracker *tracker,
    asio::thread_pool *pool,
    bool prune_remotes,
    bool prune_branches,
    std::atomic_bool &has_error) {

    auto update_action = [root,
                          repo,
                          tracker,
                          pool,
                          prune_remotes,
                          prune_branches,
                          &has_error]() {
        try {
            update_repository(
                root,
                repo,
                *tracker,
                prune_remotes,
                prune_branches);
            tracker->set_phase(
                root,
                repo->name,
                RepoPhase::SUCCEEDED,
                "Repository synced");
        } catch (const std::exception &e) {
            tracker->set_phase(
                root,
                repo->name,
                RepoPhase::FAILED,
                e.what(),
                MessageLevel::ERROR);
            has_error.store(true);
            return;
        }
        for (auto &child : repo->children) {
            sync_repository(
                root,
                &child,
                tracker,
                pool,
                prune_remotes,
                prune_branches,
                has_error);
        }
    };

    auto clone_action = [root,
                         repo,
                         tracker,
                         pool,
                         prune_remotes,
                         prune_branches,
                         &has_error]() {
        try {
            clone_repository(root, repo, *tracker);
            tracker->set_phase(
                root,
                repo->name,
                RepoPhase::SUCCEEDED,
                "Repository synced");
        } catch (const std::exception &e) {
            tracker->set_phase(
                root,
                repo->name,
                RepoPhase::FAILED,
                e.what(),
                MessageLevel::ERROR);
            has_error.store(true);
            return;
        }
        for (auto &child : repo->children) {
            sync_repository(
                root,
                &child,
                tracker,
                pool,
                prune_remotes,
                prune_branches,
                has_error);
        }
    };

    auto repo_manager = create_repo_manager(repo->type);
    auto is_repo = repo_manager->is_repo(
        (std::filesystem::path(root) / repo->name).string());
    if (is_repo) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
    }
}

int run_sync(const SyncOptions &options) {
    std::vector<Tree> config = filter_trees_by_root(
        get_dependencies(options.config_file),
        options.root_patterns);
    TrackedOperation op(config, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    std::atomic_bool has_error{false};
    auto pool = create_thread_pool(options.jobs);
    for (auto &tree : config) {
        for (auto &repo : tree.repos) {
            sync_repository(
                tree.root,
                &repo,
                &tracker,
                &pool,
                options.prune_remotes,
                options.prune_branches,
                has_error);
        }
    }
    pool.join();
    return has_error.load() ? 1 : 0;
}
