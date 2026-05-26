#include "command/remotesync.hpp"
#include "core/tracker.hpp"
#include "core/tree.hpp"
#include "persistence/config.hpp"
#include "presentation/output_view.hpp"
#include "presentation/tracked_operation.hpp"
#include "util/common.hpp"
#include "util/constants.hpp"
#include "vcs/git_manager.hpp"
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

std::vector<RepoJob> collect_repos(const std::vector<Tree> &config) {
    std::vector<RepoJob> repo_jobs;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            repo_jobs.push_back(
                RepoJob{
                    .root = tree.root,
                    .name = repo.name,
                    .path = construct_repo_path(tree.root, repo.name),
                });
        }
    }
    return repo_jobs;
}

bool sync_branch(
    Tracker &tracker,
    const std::string &root,
    const std::string &repo_name,
    const std::string &repo_path,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::string &branch,
    bool source_available,
    bool dry_run,
    std::atomic_bool &has_error,
    int timeout_seconds) {
    try {
        std::string source_label = source_remote + "/" + branch;
        const std::string &local_branch = branch;

        if (source_available) {
            try {
                const bool has_branch =
                    GitManager::branch_exists(repo_path, local_branch);
                if (!has_branch) {
                    GitManager::pull(
                        repo_path,
                        source_remote,
                        branch,
                        local_branch,
                        timeout_seconds);
                }
                GitManager::switch_branch(repo_path, local_branch);
                if (has_branch) {
                    GitManager::pull(
                        repo_path,
                        source_remote,
                        branch,
                        local_branch,
                        timeout_seconds);
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
            if (!GitManager::branch_exists(repo_path, local_branch)) {
                tracker.set_phase(
                    root,
                    repo_name,
                    RepoPhase::RUNNING,
                    "[" + branch + "] Missing source branch, skipped",
                    MessageLevel::WARNING);
                return true;
            }
            source_label = "local " + branch;
            GitManager::switch_branch(repo_path, local_branch);
        }

        bool target_exists = false;
        BranchSyncState state = BranchSyncState::SOURCE_AHEAD;
        try {
            state = GitManager::compare_branch_to_remote(
                repo_path,
                local_branch,
                target_remote,
                branch,
                timeout_seconds);
            target_exists = true;
        } catch (const std::exception &) {
            target_exists = false;
        }

        if (target_exists) {
            try {
                if (state == BranchSyncState::UP_TO_DATE) {
                    tracker.set_phase(
                        root,
                        repo_name,
                        RepoPhase::RUNNING,
                        "[" + branch + "] Already up-to-date",
                        MessageLevel::OUTPUT);
                    return true;
                }
                if (state == BranchSyncState::TARGET_AHEAD) {
                    tracker.set_phase(
                        root,
                        repo_name,
                        RepoPhase::FAILED,
                        "[" + branch +
                            "] Target is ahead; cannot push without merge",
                        MessageLevel::ERROR);
                    has_error.store(true);
                    return false;
                }
                if (state == BranchSyncState::DIVERGED) {
                    tracker.set_phase(
                        root,
                        repo_name,
                        RepoPhase::FAILED,
                        "[" + branch +
                            "] Branches diverged; manual merge required",
                        MessageLevel::ERROR);
                    has_error.store(true);
                    return false;
                }
            } catch (const std::exception &e) {
                tracker.set_phase(
                    root,
                    repo_name,
                    RepoPhase::RUNNING,
                    "[" + branch +
                        "] Could not compare: " + std::string(e.what()),
                    MessageLevel::WARNING);
            }
        }

        if (dry_run) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "[" + branch + "] DRY-RUN would push " + source_label + " to " +
                    target_remote + "/" + branch,
                MessageLevel::OUTPUT);
            return true;
        }

        GitManager::push(
            repo_path,
            target_remote,
            local_branch,
            branch,
            timeout_seconds);

        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::RUNNING,
            "[" + branch + "] Synced",
            MessageLevel::OUTPUT);
        return true;
    } catch (const std::exception &e) {
        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::FAILED,
            "[" + branch + "] " + std::string(e.what()),
            MessageLevel::ERROR);
        has_error.store(true);
        return false;
    }
}

void process_repo_sync(
    const RepoJob &repo_job,
    Tracker &tracker,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::vector<std::string> &branches,
    bool dry_run,
    std::atomic_bool &has_error,
    int timeout_seconds) {

    tracker.set_phase(
        repo_job.root,
        repo_job.name,
        RepoPhase::RUNNING,
        "Synchronizing remotes");
    try {
        if (!GitManager::is_repo(repo_job.path)) {
            tracker.set_phase(
                repo_job.root,
                repo_job.name,
                RepoPhase::FAILED,
                "Not a git repository",
                MessageLevel::ERROR);
            has_error.store(true);
            return;
        }

        if (GitManager::get_status(repo_job.path).has_changes) {
            tracker.set_phase(
                repo_job.root,
                repo_job.name,
                RepoPhase::FAILED,
                "Skipped: repository has uncommitted changes",
                MessageLevel::ERROR);
            has_error.store(true);
            return;
        }

        const auto repo_branches = GitManager::get_branches(repo_job.path);
        const auto current_it =
            std::ranges::find_if(repo_branches, [](const Branch &branch) {
                return branch.is_current;
            });
        const std::string original_branch =
            current_it == repo_branches.end() ? "" : current_it->name;

        bool source_available = true;

        bool repo_failed = false;
        for (const auto &branch : branches) {
            if (!sync_branch(
                    tracker,
                    repo_job.root,
                    repo_job.name,
                    repo_job.path,
                    source_remote,
                    target_remote,
                    branch,
                    source_available,
                    dry_run,
                    has_error,
                    timeout_seconds)) {
                repo_failed = true;
            }
        }
        if (!original_branch.empty()) {
            GitManager::switch_branch(
                repo_job.path,
                original_branch,
                SwitchMode::FORCE);
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
        has_error.store(true);
    }
}
} // namespace

int run_remotesync(const RemoteSyncOptions &options) {
    const auto trees = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns);

    TrackedOperation op(trees, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    const std::vector<RepoJob> repo_jobs = collect_repos(trees);

    std::atomic_bool has_error{false};
    auto pool = create_thread_pool(options.jobs);

    for (const auto &repo_job : repo_jobs) {
        asio::post(pool, [&, repo_job]() {
            process_repo_sync(
                repo_job,
                tracker,
                options.source_remote,
                options.target_remote,
                options.branches,
                options.dry_run,
                has_error,
                options.timeout_seconds);
        });
    }

    pool.join();

    return has_error.load() ? kFailure : 0;
}
