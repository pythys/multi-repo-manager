#include "command/update.hpp"
#include "core/tracker.hpp"
#include "core/tree.hpp"
#include "persistence/config.hpp"
#include "presentation/output_view.hpp"
#include "presentation/tracked_operation.hpp"
#include "util/common.hpp"
#include "util/constants.hpp"
#include "vcs/git_manager.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace asio = boost::asio;

int run_update(const UpdateOptions &options) {
    const auto config = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns,
        options.selector.min_depth);

    TrackedOperation op(config, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    std::atomic_bool has_error{false};
    auto pool = create_thread_pool(options.jobs);

    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto updater = [repo, tree, &tracker, &has_error, &options] {
                auto repo_path = construct_repo_path(tree.root, repo.name);
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::RUNNING,
                    "Updating repository");
                try {
                    const auto branches = GitManager::get_branches(repo_path);
                    const Branch *current = find_current_branch(branches);
                    if (!current) {
                        throw std::runtime_error(
                            "No tracked current branch in " + repo_path);
                    }
                    if (current->remote.empty()) {
                        throw std::runtime_error(
                            "Current branch has no remote in " + repo_path);
                    }
                    const auto summary_lines = GitManager::pull(
                        repo_path,
                        current->remote,
                        current->name,
                        current->name,
                        options.timeout_seconds);

                    for (const auto &line : summary_lines) {
                        tracker.set_phase(
                            tree.root,
                            repo.name,
                            RepoPhase::RUNNING,
                            line,
                            MessageLevel::OUTPUT);
                    }
                } catch (const std::exception &e) {
                    tracker.set_phase(
                        tree.root,
                        repo.name,
                        RepoPhase::FAILED,
                        e.what(),
                        MessageLevel::ERROR);
                    has_error.store(true);
                    return;
                }
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::SUCCEEDED,
                    "Finished updating repository");
            };
            asio::post(pool, updater);
        }
    }
    pool.join();
    return has_error.load() ? 1 : 0;
}
