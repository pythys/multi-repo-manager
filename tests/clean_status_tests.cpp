#include "command/sync.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <ranges>
#include <string>

namespace fs = std::filesystem;

const GitGuard git_guard;

int run_scenario(const std::string &scenario_name) {
    return run_sync(
        SyncOptions{
            .config_file = std::string(TEST_RESOURCES_DIR) + "/scenarios/" +
                           scenario_name + ".yml",
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1});
}

bool contains_status_message(
    const std::vector<std::string> &messages,
    const std::string &needle) {
    return std::ranges::any_of(messages, [&](const std::string &line) {
        return line.find(needle) != std::string::npos;
    });
}

TEST(CleanStatusTests, ReportsRepositoryStatus) {
    int result = run_scenario("status_checks");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_status_checks");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("hello-world", repos[0].name);

    const fs::path repo_path = fs::path("test_status_checks") / repos[0].name;
    if (fs::exists(repo_path)) {
        const RepoStatus status = GitManager::get_status(repo_path.string());
        EXPECT_FALSE(status.has_changes);
    }
}

TEST(CleanStatusTests, ReportsCleanRepositoryState) {
    int result = run_scenario("status_checks");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_status_checks");
    ASSERT_EQ(1, repos.size());

    const fs::path repo_path = fs::path("test_status_checks") / repos[0].name;
    if (fs::exists(repo_path)) {
        const RepoStatus status = GitManager::get_status(repo_path.string());
        EXPECT_TRUE(!status.has_changes);
    }
}