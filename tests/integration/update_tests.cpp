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

std::string get_update_scenario_path(const std::string &scenario_name) {
    return std::string(TEST_RESOURCES_DIR) + "/scenarios/sync/" +
           scenario_name + ".yml";
}

TEST(UpdateTests, TimeoutConfigurationIsRespected) {
    int result = run_update(
        UpdateOptions{
            .selector =
                RepositorySelector{
                    .config_file =
                        get_update_scenario_path("basic_repositories"),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {}},
            .jobs = 1,
            .timeout_seconds = 120});
    EXPECT_EQ(0, result);
}

TEST(UpdateTests, ZeroTimeoutDisablesTimeout) {
    int result = run_update(
        UpdateOptions{
            .selector =
                RepositorySelector{
                    .config_file =
                        get_update_scenario_path("basic_repositories"),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {}},
            .jobs = 1,
            .timeout_seconds = 0});
    EXPECT_EQ(0, result);
}

TEST(UpdateTests, DefaultTimeoutIsApplied) {
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
    int result = run_update(
        UpdateOptions{
            .selector =
                RepositorySelector{
                    .config_file = get_update_scenario_path("multiple_trees"),
                    .find_paths = {},
                    .root_patterns = {},
                    .name_patterns = {}},
            .jobs = 2,
            .timeout_seconds = 180});
    EXPECT_EQ(0, result);
}