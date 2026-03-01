#include "sync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <tbb/parallel_for_each.h>
#include <unordered_set>
#include <vector>

namespace asio = boost::asio;

namespace {
enum class MatchType : std::uint8_t { TO_REMOVE, TO_ADD };

std::vector<Remote> find_remotes(
    const std::vector<Remote> &conf_remotes,
    const std::vector<Remote> &repo_remotes,
    MatchType match_type) {

    std::vector<Remote> result;

    auto compare_by_name = [](const Remote &a, const Remote &b) {
        return a.name == b.name;
    };

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto &repo_remote : repo_remotes) {
            auto it = std::ranges::find_if(
                conf_remotes,
                [&](const Remote &tree_remote) {
                    return compare_by_name(tree_remote, repo_remote);
                });
            if (it == conf_remotes.end()) {
                result.push_back(repo_remote);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto &tree_remote : conf_remotes) {
            auto it = std::ranges::find_if(
                repo_remotes,
                [&](const Remote &repo_remote) {
                    return compare_by_name(repo_remote, tree_remote);
                });
            if (it == repo_remotes.end()) {
                result.push_back(tree_remote);
            }
        }
    }
    return result;
}

std::vector<Branch> find_branches(
    const std::vector<Branch> &conf_branches,
    const std::vector<Branch> &repo_branches,
    MatchType match_type) {

    std::vector<Branch> result;
    auto compare_by_name = [](const Branch &a, const Branch &b) {
        return a.name == b.name;
    };

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto &repo_branch : repo_branches) {
            auto it = std::ranges::find_if(
                conf_branches,
                [&](const Branch &tree_branch) {
                    return compare_by_name(tree_branch, repo_branch);
                });
            if (it == conf_branches.end()) {
                result.push_back(repo_branch);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto &tree_branch : conf_branches) {
            auto it = std::ranges::find_if(
                repo_branches,
                [&](const Branch &repo_branch) {
                    return compare_by_name(repo_branch, tree_branch);
                });
            if (it == repo_branches.end()) {
                result.push_back(tree_branch);
            }
        }
    }
    return result;
}

const Branch *find_current_branch(const std::vector<Branch> &branches) {
    auto it = std::ranges::find_if(branches, [](const Branch &branch) {
        return branch.is_current;
    });
    return it == branches.end() ? nullptr : &(*it);
}

void set_current_branch(
    const std::string &path,
    const std::vector<Branch> &repo_branches,
    const std::vector<Branch> &conf_branches,
    RepoManager *repo_manager) {

    const Branch *repo_current = find_current_branch(repo_branches);
    const Branch *conf_current = find_current_branch(conf_branches);

    if (!conf_current || !repo_current ||
        repo_current->name == conf_current->name) {
        return;
    }

    const bool has_conf_branch =
        std::ranges::any_of(repo_branches, [&](const Branch &branch) {
            return branch.name == conf_current->name;
        });
    if (!has_conf_branch) {
        return;
    }

    repo_manager->checkout_branch(path, conf_current->name);
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
    auto to_add_branches =
        find_branches(desired_branches, repo_branches, MatchType::TO_ADD);
    std::unordered_set<std::string> remotes_to_fetch;
    for (const auto &branch : to_add_branches) {
        if (!branch.remote.empty()) {
            remotes_to_fetch.insert(branch.remote);
        }
    }
    for (const auto &remote_name : remotes_to_fetch) {
        repo_manager->fetch_remote(repo_path, remote_name);
    }
    for (const auto &branch : to_add_branches) {
        repo_manager->add_branch(repo_path, branch);
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
    repo_branches = repo_manager->get_branches(repo_path);
    set_current_branch(
        repo_path,
        repo_branches,
        desired_branches,
        repo_manager);
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
    const std::string repo_path = root + "/" + repo->name;
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
        repo_manager->copy(it->url, root + "/" + repo->name);
    } else {
        throw std::runtime_error(
            "No remote found with name 'origin' for repo: " + repo->name);
    }
    for (size_t i = 0; i < repo->remotes.size(); i++) {
        if (repo->remotes[i].name == "origin") {
            continue;
        }
        repo_manager->add_remote(root + "/" + repo->name, repo->remotes[i]);
    }
    sync_branches(
        tracker,
        root,
        repo->name,
        repo_manager.get(),
        root + "/" + repo->name,
        repo->branches,
        false);
}

void sync_repository(
    const std::string &root,
    Repo *repo,
    Tracker *tracker,
    asio::thread_pool *pool,
    bool prune_remotes,
    bool prune_branches) {

    auto update_action =
        [root, repo, tracker, pool, prune_remotes, prune_branches]() {
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
                return;
            }
            for (auto &child : repo->children) {
                sync_repository(
                    root,
                    &child,
                    tracker,
                    pool,
                    prune_remotes,
                    prune_branches);
            }
        };

    auto clone_action =
        [root, repo, tracker, pool, prune_remotes, prune_branches]() {
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
                return;
            }
            for (auto &child : repo->children) {
                sync_repository(
                    root,
                    &child,
                    tracker,
                    pool,
                    prune_remotes,
                    prune_branches);
            }
        };

    auto repo_manager = create_repo_manager(repo->type);
    auto is_repo = repo_manager->is_repo(root + "/" + repo->name);
    if (is_repo) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
    }
}

int run_sync(
    const std::string &config_file,
    int pool_size,
    bool prune_remotes,
    bool prune_branches,
    const std::vector<std::string> &root_patterns) {
    std::vector<Tree> config =
        filter_trees_by_root(get_dependencies(config_file), root_patterns);
    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(detect_output_mode(), tracker);
    view->start();

    const auto effective_pool_size =
        static_cast<std::size_t>(pool_size > 0 ? pool_size : SYNC_POOL_SIZE);
    asio::thread_pool pool(effective_pool_size);
    for (auto &tree : config) {
        for (auto &repo : tree.repos) {
            sync_repository(
                tree.root,
                &repo,
                &tracker,
                &pool,
                prune_remotes,
                prune_branches);
        }
    }
    pool.join();
    tracker.close();
    view->stop();
    return 0;
}
