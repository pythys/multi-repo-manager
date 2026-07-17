#include "command/status.hpp"
#include "core/tracker.hpp"
#include "core/tree.hpp"
#include "persistence/config.hpp"
#include "presentation/output_view.hpp"
#include "presentation/tracked_operation.hpp"
#include "util/common.hpp"
#include "vcs/git_manager.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace asio = boost::asio;

int run_status(const StatusOptions &options) {
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
            auto status_checker = [repo,
                                   tree,
                                   &tracker,
                                   &has_error,
                                   modified_only = options.modified_only] {
                auto repo_path = construct_repo_path(tree.root, repo.name);
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::RUNNING,
                    "Collecting status");
                try {
                    const auto status = GitManager::get_status(repo_path);
                    for (const auto &message : status.messages) {
                        tracker.set_phase(
                            tree.root,
                            repo.name,
                            RepoPhase::RUNNING,
                            message,
                            MessageLevel::OUTPUT);
                    }
                    tracker.set_phase(
                        tree.root,
                        repo.name,
                        RepoPhase::SUCCEEDED,
                        "Status collected");
                    if (modified_only && !status.has_changes) {
                        tracker.remove_repo(tree.root, repo.name);
                    }
                } catch (const std::exception &e) {
                    tracker.set_phase(
                        tree.root,
                        repo.name,
                        RepoPhase::FAILED,
                        e.what(),
                        MessageLevel::ERROR);
                    has_error.store(true);
                }
            };
            asio::post(pool, status_checker);
        }
    }

    pool.join();
    return has_error.load() ? 1 : 0;
}
