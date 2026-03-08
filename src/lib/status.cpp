#include "status.hpp"
#include "config.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <string>
#include <vector>

int run_status(
    const std::string &config_file,
    const std::vector<std::string> &root_patterns) {
    const std::vector<Tree> config =
        filter_trees_by_root(get_config(config_file), root_patterns);
    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(detect_output_mode(), tracker);
    view->start();

    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = tree.root + "/" + repo.name;
            tracker.set_phase(
                tree.root,
                repo.name,
                RepoPhase::RUNNING,
                "Collecting status");
            try {
                const auto statuses = repo_manager->get_status(repo_path);
                for (const auto &status : statuses) {
                    tracker.set_phase(
                        tree.root,
                        repo.name,
                        RepoPhase::RUNNING,
                        status);
                }
            } catch (const std::exception &e) {
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::FAILED,
                    e.what(),
                    MessageLevel::ERROR);
                continue;
            }
            tracker.set_phase(
                tree.root,
                repo.name,
                RepoPhase::SUCCEEDED,
                "Status collected");
        }
    }

    tracker.close();
    view->stop();
    return 0;
}
