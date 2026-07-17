#include "command/sync.hpp"
#include "command/update.hpp"
#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "util/command_options.hpp"
#include "util/runtime.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <gtest/gtest.h>

const GitGuard git_guard;

constexpr int TIMEOUT_LONG = 180;

std::string get_update_scenario_path(const std::string &scenario_name) {
    return std::string(TEST_RESOURCES_DIR) + "/scenarios/sync/" +
           scenario_name + ".yml";
}

int prepare_repos(const std::string &scenario_name) {
    return run_sync(
        SyncOptions{
            .config_file = get_update_scenario_path(scenario_name),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1});
}

TEST(UpdateTests, DefaultTimeoutIsApplied) {
    test_utils::ScopedTempCwd scratch("mrm-update-default-timeout");
    ASSERT_EQ(0, prepare_repos("basic_repositories"));
    int result = run_update(
        UpdateOptions{
            .selector =
                RepositorySelector{
                    .config_file =
                        get_update_scenario_path("basic_repositories"),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {}},
            .jobs = 1});
    EXPECT_EQ(0, result);
}

TEST(UpdateTests, CustomTimeoutWorksWithMultipleRepos) {
    test_utils::ScopedTempCwd scratch("mrm-update-custom-timeout");
    ASSERT_EQ(0, prepare_repos("multiple_trees"));
    int result = run_update(
        UpdateOptions{
            .selector =
                RepositorySelector{
                    .config_file = get_update_scenario_path("multiple_trees"),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {}},
            .jobs = 2,
            .timeout_seconds = TIMEOUT_LONG});
    EXPECT_EQ(0, result);
}
