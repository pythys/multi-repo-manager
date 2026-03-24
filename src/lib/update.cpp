#include "update.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
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

    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(
        detect_output_mode(),
        DisplayFormat::PROGRESS,
        tracker);
    view->start();

    const auto effective_pool_size = static_cast<std::size_t>(
        options.jobs > 0 ? options.jobs : SYNC_POOL_SIZE);
    asio::thread_pool pool(effective_pool_size);
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto updater = [repo, tree, &tracker] {
                auto repo_manager = create_repo_manager(repo.type);
                auto repo_path =
                    (std::filesystem::path(tree.root) / repo.name).string();
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::RUNNING,
                    "Updating repository");
                try {
                    const auto branches = repo_manager->get_branches(repo_path);
                    const Branch *current = nullptr;
                    for (const auto &branch : branches) {
                        if (branch.is_current) {
                            current = &branch;
                            break;
                        }
                    }
                    if (!current) {
                        throw std::runtime_error(
                            "No tracked current branch in " + repo_path);
                    }
                    if (current->remote.empty()) {
                        throw std::runtime_error(
                            "Current branch has no remote in " + repo_path);
                    }
                    const auto result = repo_manager->pull_branch(
                        repo_path,
                        current->remote,
                        current->name,
                        current->name);
                    if (result.up_to_date) {
                        tracker.set_phase(
                            tree.root,
                            repo.name,
                            RepoPhase::RUNNING,
                            "Already up to date",
                            MessageLevel::OUTPUT);
                    } else if (result.old_commit.empty()) {
                        tracker.set_phase(
                            tree.root,
                            repo.name,
                            RepoPhase::RUNNING,
                            "New branch at " + result.new_commit,
                            MessageLevel::OUTPUT);
                    } else {
                        tracker.set_phase(
                            tree.root,
                            repo.name,
                            RepoPhase::RUNNING,
                            "Updating " + result.old_commit + ".." +
                                result.new_commit,
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
    tracker.close();
    view->stop();
    return 0;
}
