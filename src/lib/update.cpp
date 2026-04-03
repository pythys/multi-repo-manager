#include "update.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include "utils.hpp"
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace asio = boost::asio;

int run_update(const UpdateOptions &options) {
    const auto config = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns);

    TrackedOperation op(config, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    auto pool = create_thread_pool(options.jobs);
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto updater = [repo, tree, &tracker] {
                auto repo_manager = create_repo_manager(repo.type);
                auto repo_path = construct_repo_path(tree.root, repo.name);
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::RUNNING,
                    "Updating repository");
                try {
                    const auto branches = repo_manager->get_branches(repo_path);
                    const Branch *current = find_current_branch(branches);
                    if (!current) {
                        throw std::runtime_error(
                            "No tracked current branch in " + repo_path);
                    }
                    if (current->remote.empty()) {
                        throw std::runtime_error(
                            "Current branch has no remote in " + repo_path);
                    }
                    const auto summary_lines = repo_manager->pull_branch(
                        repo_path,
                        current->remote,
                        current->name,
                        current->name);

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
    return 0;
}
