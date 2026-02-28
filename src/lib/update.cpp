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

int run_update(const std::string &config_file) {
    const std::vector<Tree> config = get_config(config_file);
    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(detect_output_mode(), tracker);
    view->start();

    asio::thread_pool pool(SYNC_POOL_SIZE);
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
                    repo_manager->update(repo_path);
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
