#include "remotesync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "repo_manager.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <algorithm>
#include <atomic>
#include <boost/asio.hpp>
#include <string>
#include <vector>

namespace asio = boost::asio;

namespace {
constexpr int kFailure = 1;

struct RepoJob {
    std::string root;
    std::string name;
    std::string path;
};

bool sync_branch(
    Tracker &tracker,
    RepoManager *repo_manager,
    const std::string &root,
    const std::string &repo_name,
    const std::string &repo_path,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::string &branch,
    bool source_available,
    bool dry_run,
    std::atomic_bool &has_operational_error) {
    try {
        std::string source_label = source_remote + "/" + branch;
        if (source_available) {
            try {
                const bool has_branch =
                    repo_manager->branch_exists(repo_path, branch);
                if (!has_branch) {
                    repo_manager
                        ->pull_branch(repo_path, source_remote, branch, branch);
                }
                repo_manager->switch_branch(repo_path, branch);
                if (has_branch) {
                    repo_manager
                        ->pull_branch(repo_path, source_remote, branch, branch);
                }
            } catch (const std::exception &) {
                source_available = false;
                tracker.set_phase(
                    root,
                    repo_name,
                    RepoPhase::RUNNING,
                    "Source fetch failed; using local branch fallback",
                    MessageLevel::WARNING);
            }
        }

        if (!source_available) {
            if (!repo_manager->branch_exists(repo_path, branch)) {
                tracker.set_phase(
                    root,
                    repo_name,
                    RepoPhase::RUNNING,
                    "[" + branch + "] Missing source branch, skipped",
                    MessageLevel::WARNING);
                return true;
            }
            source_label = "local " + branch;
            repo_manager->switch_branch(repo_path, branch);
        }

        if (dry_run) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] DRY-RUN would push " + source_label + " to " +
                    target_remote + "/" + branch);
            return true;
        }

        repo_manager->push_branch(repo_path, target_remote, branch, branch);

        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::RUNNING,
            "[" + branch + "] Synced");
        return true;
    } catch (const std::exception &e) {
        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::FAILED,
            "[" + branch + "] " + std::string(e.what()),
            MessageLevel::ERROR);
        has_operational_error.store(true);
        return false;
    }
}
} // namespace

int run_remotesync(
    const std::string &config_file,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::vector<std::string> &branches,
    bool dry_run,
    const std::vector<std::string> &root_patterns,
    int jobs) {
    const std::vector<Tree> config =
        filter_trees_by_root(get_config(config_file), root_patterns);
    Tracker tracker;
    tracker.populate(config);
    auto view = create_output_view(detect_output_mode(), tracker);
    view->start();

    std::vector<RepoJob> repo_jobs;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            if (repo.type == RepoType::GIT) {
                repo_jobs.push_back(
                    RepoJob{
                        .root = tree.root,
                        .name = repo.name,
                        .path = tree.root + "/" + repo.name,
                    });
            }
        }
    }

    std::atomic_bool has_operational_error{false};
    const auto effective_jobs =
        static_cast<std::size_t>(jobs > 0 ? jobs : SYNC_POOL_SIZE);

    asio::thread_pool pool(effective_jobs);

    for (const auto &repo_job : repo_jobs) {
        asio::post(pool, [&, repo_job]() {
            std::unique_ptr<RepoManager> repo_manager =
                create_repo_manager(RepoType::GIT);
            tracker.set_phase(
                repo_job.root,
                repo_job.name,
                RepoPhase::RUNNING,
                "Synchronizing remotes");
            try {
                if (!repo_manager->is_repo(repo_job.path)) {
                    tracker.set_phase(
                        repo_job.root,
                        repo_job.name,
                        RepoPhase::FAILED,
                        "Not a git repository",
                        MessageLevel::ERROR);
                    has_operational_error.store(true);
                    return;
                }

                const auto repo_branches =
                    repo_manager->get_branches(repo_job.path);
                const auto current_it = std::ranges::find_if(
                    repo_branches,
                    [](const Branch &branch) { return branch.is_current; });
                const std::string original_branch =
                    current_it == repo_branches.end() ? "" : current_it->name;

                bool source_available = true;

                bool repo_failed = false;
                for (const auto &branch : branches) {
                    if (!sync_branch(
                            tracker,
                            repo_manager.get(),
                            repo_job.root,
                            repo_job.name,
                            repo_job.path,
                            source_remote,
                            target_remote,
                            branch,
                            source_available,
                            dry_run,
                            has_operational_error)) {
                        repo_failed = true;
                    }
                }
                if (!original_branch.empty()) {
                    repo_manager->switch_branch(repo_job.path, original_branch);
                }
                if (!repo_failed) {
                    tracker.set_phase(
                        repo_job.root,
                        repo_job.name,
                        RepoPhase::SUCCEEDED,
                        "Remote sync finished");
                }
            } catch (const std::exception &e) {
                tracker.set_phase(
                    repo_job.root,
                    repo_job.name,
                    RepoPhase::FAILED,
                    e.what(),
                    MessageLevel::ERROR);
                has_operational_error.store(true);
            }
        });
    }

    pool.join();

    tracker.close();
    view->stop();
    return has_operational_error.load() ? kFailure : 0;
}
