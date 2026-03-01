#include "remotesync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "repo_factory.hpp"
#include "repo_manager.hpp"
#include "tree.hpp"
#include <atomic>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <tbb/global_control.h>
#include <tbb/parallel_for_each.h>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr int kFailure = 1;

void log_line(
    std::ostream &stream,
    std::mutex &log_mutex,
    const std::string &message) {
    std::scoped_lock lock(log_mutex);
    stream << message << "\n";
}

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

void sync_branch(
    RepoManager *repo_manager,
    std::mutex &log_mutex,
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
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch +
                    "] SKIPPED - Missing source branch");
            return;
        }

        const std::string target_ref =
            "refs/remotes/" + target_remote + "/" + branch;
        if (!repo_manager->ref_exists(repo_path, target_ref)) {
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch +
                    "] SKIPPED - Missing target branch");
            return;
        }

        const RefSyncState decision =
            repo_manager->compare_refs(repo_path, *source_ref, target_ref);
        if (decision == RefSyncState::UP_TO_DATE) {
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch + "] UP-TO-DATE");
            return;
        }
        if (decision == RefSyncState::TARGET_AHEAD) {
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch + "] SKIPPED - Target ahead");
            return;
        }
        if (decision == RefSyncState::DIVERGED) {
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch + "] SKIPPED - Diverged");
            return;
        }

        if (dry_run) {
            log_line(
                std::cout,
                log_mutex,
                "[" + repo_path + ":" + branch + "] DRY-RUN - Would push " +
                    *source_ref + " to " + target_remote + "/" + branch);
            return;
        }

        const std::string target_branch_ref = "refs/heads/" + branch;
        repo_manager->push_ref(
            repo_path,
            target_remote,
            *source_ref,
            target_branch_ref);

        log_line(
            std::cout,
            log_mutex,
            "[" + repo_path + ":" + branch + "] SYNCED");
    } catch (const std::exception &e) {
        log_line(
            std::cerr,
            log_mutex,
            "[" + repo_path + ":" + branch + "] ERROR - " +
                std::string(e.what()));
        has_operational_error.store(true);
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
    std::vector<std::string> repo_paths;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            if (repo.type == RepoType::GIT) {
                repo_paths.push_back(tree.root + "/" + repo.name);
            }
        }
    }

    std::atomic_bool has_operational_error{false};
    std::mutex log_mutex;
    const auto effective_jobs =
        static_cast<std::size_t>(jobs > 0 ? jobs : SYNC_POOL_SIZE);
    tbb::global_control control(
        tbb::global_control::max_allowed_parallelism,
        effective_jobs);

    tbb::parallel_for_each(repo_paths, [&](const std::string &repo_path) {
        std::unique_ptr<RepoManager> repo_manager =
            create_repo_manager(RepoType::GIT);
        try {
            if (!fs::exists(repo_path)) {
                log_line(
                    std::cerr,
                    log_mutex,
                    "[" + repo_path + "] ERROR - Missing path");
                has_operational_error.store(true);
                return;
            }

            if (!repo_manager->is_repo(repo_path)) {
                log_line(
                    std::cerr,
                    log_mutex,
                    "[" + repo_path + "] ERROR - Not a git repository");
                has_operational_error.store(true);
                return;
            }

            try {
                repo_manager->fetch_remote(repo_path, target_remote);
            } catch (const std::exception &e) {
                log_line(
                    std::cerr,
                    log_mutex,
                    "[" + repo_path +
                        "] ERROR - Failed to fetch target remote " +
                        target_remote + ": " + e.what());
                has_operational_error.store(true);
                return;
            }

            bool source_available = true;
            try {
                repo_manager->fetch_remote(repo_path, source_remote);
            } catch (const std::exception &) {
                source_available = false;
            }

            for (const auto &branch : branches) {
                sync_branch(
                    repo_manager.get(),
                    log_mutex,
                    repo_path,
                    source_remote,
                    target_remote,
                    branch,
                    source_available,
                    dry_run,
                    has_operational_error);
            }
        } catch (const std::exception &e) {
            log_line(
                std::cerr,
                log_mutex,
                "[" + repo_path + "] ERROR - " + std::string(e.what()));
            has_operational_error.store(true);
        }
    });

    return has_operational_error.load() ? kFailure : 0;
}
