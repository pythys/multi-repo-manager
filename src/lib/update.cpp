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

int run_update(
    const std::string &config_file,
    int pool_size,
    const std::vector<std::string> &root_patterns) {
    const std::vector<Tree> config =
        filter_trees_by_root(get_config(config_file), root_patterns);
    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(detect_output_mode(), tracker);
    view->start();

    const auto effective_pool_size =
        static_cast<std::size_t>(pool_size > 0 ? pool_size : SYNC_POOL_SIZE);
    asio::thread_pool pool(effective_pool_size);
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto updater = [repo, tree, &tracker] {
                auto repo_manager = create_repo_manager(repo.type);
                auto repo_path = tree.root + "/" + repo.name;
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
                    repo_manager->pull_branch(
                        repo_path,
                        current->remote,
                        current->name,
                        current->name);
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
