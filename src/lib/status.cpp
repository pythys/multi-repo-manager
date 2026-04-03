#include "status.hpp"
#include "config.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include "utils.hpp"
#include <string>
#include <vector>

int run_status(const StatusOptions &options) {
    const auto config = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns);

    TrackedOperation op(config, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    bool has_error = false;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = construct_repo_path(tree.root, repo.name);
            tracker.set_phase(
                tree.root,
                repo.name,
                RepoPhase::RUNNING,
                "Collecting status");
            try {
                const auto status = repo_manager->get_status(repo_path);
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
                if (options.modified_only && !status.has_changes) {
                    tracker.remove_repo(tree.root, repo.name);
                }
            } catch (const std::exception &e) {
                tracker.set_phase(
                    tree.root,
                    repo.name,
                    RepoPhase::FAILED,
                    e.what(),
                    MessageLevel::ERROR);
                has_error = true;
                continue;
            }
        }
    }

    return has_error ? 1 : 0;
}
