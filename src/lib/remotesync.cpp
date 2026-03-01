#include "remotesync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "output_view.hpp"
#include "repo_factory.hpp"
#include "repo_manager.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <atomic>
#include <filesystem>
#include <optional>
#include <string>
#include <tbb/global_control.h>
#include <tbb/parallel_for_each.h>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr int kFailure = 1;

struct RepoJob {
    std::string root;
    std::string name;
    std::string path;
};

std::optional<std::string> source_ref_for_branch(
    RepoManager *repo_manager,
    const std::string &repo_path,
    const std::string &source_remote,
    const std::string &branch,
    bool source_available) {
    std::string source_remote_ref =
        "refs/remotes/" + source_remote + "/" + branch;
    if (source_available &&
        repo_manager->ref_exists(repo_path, source_remote_ref)) {
        return source_remote_ref;
    }

    std::string local_ref = "refs/heads/" + branch;
    if (repo_manager->ref_exists(repo_path, local_ref)) {
        return local_ref;
    }

    return std::nullopt;
}

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
        const std::optional<std::string> source_ref = source_ref_for_branch(
            repo_manager,
            repo_path,
            source_remote,
            branch,
            source_available);
        if (!source_ref.has_value()) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] Missing source branch, skipped",
                MessageLevel::WARNING);
            return true;
        }

        const std::string target_ref =
            "refs/remotes/" + target_remote + "/" + branch;
        if (!repo_manager->ref_exists(repo_path, target_ref)) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] Missing target branch, skipped",
                MessageLevel::WARNING);
            return true;
        }

        const RefSyncState decision =
            repo_manager->compare_refs(repo_path, *source_ref, target_ref);
        if (decision == RefSyncState::UP_TO_DATE) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] Up to date");
            return true;
        }
        if (decision == RefSyncState::TARGET_AHEAD) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] Target ahead, skipped",
                MessageLevel::WARNING);
            return true;
        }
        if (decision == RefSyncState::DIVERGED) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] Diverged, skipped",
                MessageLevel::WARNING);
            return true;
        }

        if (dry_run) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] DRY-RUN would push " + *source_ref + " to " +
                    target_remote + "/" + branch);
            return true;
        }

        const std::string target_branch_ref = "refs/heads/" + branch;
        repo_manager->push_ref(
            repo_path,
            target_remote,
            *source_ref,
            target_branch_ref);

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
                    RepoJob{tree.root, repo.name, tree.root + "/" + repo.name});
            }
        }
    }

    std::atomic_bool has_operational_error{false};
    const auto effective_jobs =
        static_cast<std::size_t>(jobs > 0 ? jobs : SYNC_POOL_SIZE);
    tbb::global_control control(
        tbb::global_control::max_allowed_parallelism,
        effective_jobs);

    tbb::parallel_for_each(repo_jobs, [&](const RepoJob &repo_job) {
        std::unique_ptr<RepoManager> repo_manager =
            create_repo_manager(RepoType::GIT);
        tracker.set_phase(
            repo_job.root,
            repo_job.name,
            RepoPhase::RUNNING,
            "Synchronizing remotes");
        try {
            if (!fs::exists(repo_job.path)) {
                tracker.set_phase(
                    repo_job.root,
                    repo_job.name,
                    RepoPhase::FAILED,
                    "Missing path",
                    MessageLevel::ERROR);
                has_operational_error.store(true);
                return;
            }

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

            try {
                repo_manager->fetch_remote(repo_job.path, target_remote);
            } catch (const std::exception &e) {
                tracker.set_phase(
                    repo_job.root,
                    repo_job.name,
                    RepoPhase::FAILED,
                    "Failed to fetch target remote " + target_remote + ": " +
                        e.what(),
                    MessageLevel::ERROR);
                has_operational_error.store(true);
                return;
            }

            bool source_available = true;
            try {
                repo_manager->fetch_remote(repo_job.path, source_remote);
            } catch (const std::exception &) {
                source_available = false;
                tracker.set_phase(
                    repo_job.root,
                    repo_job.name,
                    RepoPhase::RUNNING,
                    "Source fetch failed; using local branch fallback",
                    MessageLevel::WARNING);
            }

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

    tracker.close();
    view->stop();
    return has_operational_error.load() ? kFailure : 0;
}
