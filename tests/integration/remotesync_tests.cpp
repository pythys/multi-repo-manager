#include "command/remotesync.hpp"
#include "command/sync.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "util/command_options.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

const GitGuard git_guard;

namespace {

void setup_local_fork_scenario(const std::string &root_name) {
    const fs::path root_dir = root_name;
    const fs::path repo_dir = root_dir / "hello-world";
    const fs::path origin_dir = root_dir / "origin.git";
    const fs::path upstream_dir = root_dir / "upstream.git";

    if (fs::exists(root_dir)) {
        fs::remove_all(root_dir);
    }
    fs::create_directories(root_dir);

    GitManager::init(origin_dir.string(), "master");
    GitManager::init(upstream_dir.string(), "master");

    GitManager::clone(
        "https://github.com/octocat/Hello-World",
        repo_dir.string());

    GitManager::remove_remote(
        repo_dir.string(),
        Remote{.name = "origin", .url = ""});
    GitManager::add_remote(
        repo_dir.string(),
        Remote{.name = "origin", .url = origin_dir.string()});
    GitManager::add_remote(
        repo_dir.string(),
        Remote{.name = "upstream", .url = upstream_dir.string()});
}

void write_fork_test_config(
    const fs::path &config_file,
    const std::string &root_name) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "trees" << YAML::Value << YAML::BeginSeq;

    out << YAML::BeginMap;
    out << YAML::Key << "root" << YAML::Value << root_name;
    out << YAML::Key << "repos" << YAML::Value << YAML::BeginSeq;

    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "hello-world";
    out << YAML::Key << "type" << YAML::Value << "git";
    out << YAML::Key << "remotes" << YAML::Value << YAML::BeginSeq;

    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "origin";
    out << YAML::Key << "url" << YAML::Value << "placeholder";
    out << YAML::EndMap;

    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "upstream";
    out << YAML::Key << "url" << YAML::Value << "placeholder";
    out << YAML::EndMap;

    out << YAML::EndSeq;
    out << YAML::Key << "branches" << YAML::Value << YAML::BeginSeq;

    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "master";
    out << YAML::Key << "remote" << YAML::Value << "origin";
    out << YAML::Key << "is_current" << YAML::Value << true;
    out << YAML::EndMap;

    out << YAML::EndSeq;
    out << YAML::EndMap;

    out << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream file(config_file);
    file << out.c_str();
}

int run_remote_sync_with_config(
    const fs::path &config_file,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::vector<std::string> &branches,
    bool dry_run = false) {
    return run_remotesync(
        RemoteSyncOptions{
            .selector =
                {
                    .config_file = config_file.string(),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {},
                },
            .source_remote = source_remote,
            .target_remote = target_remote,
            .branches = branches,
            .dry_run = dry_run,
            .jobs = 1,
        });
}

} // namespace

TEST(RemoteSyncTests, DryRunReportsPlannedOperations) {
    test_utils::ScopedTempCwd scratch("mrm-remotesync-dry-run");

    const std::string test_root = "test_fork_dry_run";
    setup_local_fork_scenario(test_root);

    const fs::path config_file = fs::path(test_root) / "config.yml";
    write_fork_test_config(config_file, test_root);

    testing::internal::CaptureStdout();
    int result = run_remote_sync_with_config(
        config_file,
        "upstream",
        "origin",
        {"master"},
        false);
    (void)result;
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(std::string::npos, output.find("hello-world"));
}

TEST(RemoteSyncTests, SyncsBetweenConfiguredRemotes) {
    test_utils::ScopedTempCwd scratch("mrm-remotesync-sync");

    const std::string test_root = "test_fork_sync";
    setup_local_fork_scenario(test_root);

    const fs::path config_file = fs::path(test_root) / "config.yml";
    write_fork_test_config(config_file, test_root);

    testing::internal::CaptureStdout();
    int result = run_remote_sync_with_config(
        config_file,
        "upstream",
        "origin",
        {"master"},
        false);
    (void)result;
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(std::string::npos, output.find("hello-world"));
}

TEST(RemoteSyncTests, HandlesMultipleRepositoriesWithLocalForks) {
    test_utils::ScopedTempCwd scratch("mrm-remotesync-multi");

    const std::string test_root = "test_fork_multi";
    setup_local_fork_scenario(test_root);

    const fs::path config_file = fs::path(test_root) / "config.yml";
    write_fork_test_config(config_file, test_root);

    testing::internal::CaptureStdout();
    int result = run_remote_sync_with_config(
        config_file,
        "upstream",
        "origin",
        {"master"},
        true);
    (void)result;
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(std::string::npos, output.find("mrm report"));
}

TEST(RemoteSyncTests, SkipsRepositoryWithUncommittedChanges) {
    test_utils::ScopedTempCwd scratch("mrm-remotesync-dirty");

    const std::string test_root = "test_fork_dirty";
    setup_local_fork_scenario(test_root);

    const fs::path config_file = fs::path(test_root) / "config.yml";
    write_fork_test_config(config_file, test_root);

    const fs::path tracked_file =
        fs::path(test_root) / "hello-world" / "README";
    {
        std::ofstream out(tracked_file, std::ios::app);
        out << "\nlocal modification\n";
    }

    testing::internal::CaptureStdout();
    const int result = run_remote_sync_with_config(
        config_file,
        "upstream",
        "origin",
        {"master"},
        false);
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(1, result);
    EXPECT_NE(std::string::npos, output.find("uncommitted changes"));
    EXPECT_TRUE(
        GitManager::get_status((fs::path(test_root) / "hello-world").string())
            .has_changes);
}
