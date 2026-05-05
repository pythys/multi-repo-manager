#include "command/sync.hpp"
#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "util/command_options.hpp"
#include "util/runtime.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <ranges>
#include <unordered_map>
#include <vector>

const GitGuard git_guard;

std::string get_scenario_path(const std::string &scenario_name) {
    static const std::unordered_map<std::string, std::string> scenario_paths = {
        {"basic_sync", "scenarios/sync/basic_repositories.yml"},
        {"nested_repos", "scenarios/sync/nested_structure.yml"},
        {"multi_tree", "scenarios/sync/multiple_trees.yml"},
        {"branch_config", "scenarios/branch/configuration.yml"},
        {"upstream_change", "scenarios/sync/upstream_changes.yml"},
        {"branch_switching", "scenarios/branch/switching.yml"},
        {"pruning_test", "scenarios/sync/repository_pruning.yml"}};

    auto it = scenario_paths.find(scenario_name);
    if (it != scenario_paths.end()) {
        return std::string(TEST_RESOURCES_DIR) + "/" + it->second;
    }
    return std::string(TEST_RESOURCES_DIR) + "/scenarios/" + scenario_name +
           ".yml";
}

int run_scenario(const std::string &scenario_name) {
    return run_sync(
        SyncOptions{
            .config_file = get_scenario_path(scenario_name),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1});
}

int run_scenario_with_pruning(
    const std::string &scenario_name,
    bool prune_remotes,
    bool prune_branches,
    bool prune_repos) {
    return run_sync(
        SyncOptions{
            .config_file = get_scenario_path(scenario_name),
            .root_patterns = {},
            .prune_remotes = prune_remotes,
            .prune_branches = prune_branches,
            .prune_repos = prune_repos,
            .jobs = 1});
}

TEST(SyncTests, BasicSyncFunctionality) {
    int result = run_scenario("basic_sync");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_basic");
    EXPECT_EQ(2, repos.size());

    bool found_hello_world = false;
    bool found_spoon_knife = false;

    for (const auto &repo : repos) {
        if (repo.name == "hello-world") {
            found_hello_world = true;
            EXPECT_FALSE(repo.branches.empty());
            EXPECT_EQ("master", repo.branches[0].name);
            EXPECT_EQ("origin", repo.branches[0].remote);
        }
        if (repo.name == "spoon-knife") {
            found_spoon_knife = true;
            EXPECT_FALSE(repo.branches.empty());
            EXPECT_EQ("main", repo.branches[0].name);
            EXPECT_EQ("origin", repo.branches[0].remote);
        }
    }

    EXPECT_TRUE(found_hello_world);
    EXPECT_TRUE(found_spoon_knife);
}

TEST(SyncTests, NestedRepositoryStructure) {
    int result = run_scenario("nested_repos");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_nested");
    EXPECT_EQ(3, repos.size());

    bool found_parent = false;
    bool found_child1 = false;
    bool found_child2 = false;

    for (const auto &repo : repos) {
        if (repo.name == "parent")
            found_parent = true;
        if (repo.name == "parent/child1")
            found_child1 = true;
        if (repo.name == "parent/child2")
            found_child2 = true;
    }

    EXPECT_TRUE(found_parent);
    EXPECT_TRUE(found_child1);
    EXPECT_TRUE(found_child2);
}

TEST(SyncTests, MultipleWorkspaceTrees) {
    int result = run_scenario("multi_tree");
    EXPECT_EQ(0, result);

    std::vector<Repo> first_repos = find_repos("first_workspace");
    EXPECT_EQ(1, first_repos.size());
    EXPECT_EQ("hello-world", first_repos[0].name);

    std::vector<Repo> second_repos = find_repos("second_workspace");
    EXPECT_EQ(2, second_repos.size());
}

TEST(SyncTests, BranchConfigurationIsValidated) {
    int result = run_scenario("branch_config");
    EXPECT_EQ(0, result);

    std::vector<Tree> trees = get_config(
        std::string(TEST_RESOURCES_DIR) +
        "/scenarios/branch/configuration.yml");
    ASSERT_EQ(trees.size(), 1);
    ASSERT_EQ(trees[0].repos.size(), 1);
    const Repo &expected = trees[0].repos[0];

    std::vector<Repo> repos = find_repos("test_branch_config_unique");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("hello-world", repos[0].name);

    for (const auto &expected_branch : expected.branches) {
        auto actual_branch_it =
            std::ranges::find_if(repos[0].branches, [&](const Branch &b) {
                return b.name == expected_branch.name;
            });

        ASSERT_NE(actual_branch_it, repos[0].branches.end())
            << "Branch " << expected_branch.name << " not found";
        EXPECT_EQ(actual_branch_it->remote, expected_branch.remote)
            << "Branch " << expected_branch.name << " has wrong remote";
    }

    auto current_branch_it =
        std::ranges::find_if(repos[0].branches, [](const Branch &b) {
            return b.is_current;
        });
    ASSERT_NE(current_branch_it, repos[0].branches.end());
    EXPECT_EQ("master", current_branch_it->name);

    bool has_origin =
        std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
            return r.name == "origin";
        });
    bool has_upstream =
        std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
            return r.name == "upstream";
        });
    EXPECT_TRUE(has_origin);
    EXPECT_TRUE(has_upstream);
}

TEST(SyncTests, UpstreamTrackingIsUpdated) {
    int result = run_scenario("upstream_change");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_upstream_tracking");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("hello-world", repos[0].name);

    auto master_branch_it =
        std::ranges::find_if(repos[0].branches, [](const Branch &b) {
            return b.name == "master";
        });
    ASSERT_NE(master_branch_it, repos[0].branches.end());

    EXPECT_TRUE(master_branch_it->is_current);

    bool has_origin =
        std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
            return r.name == "origin";
        });
    bool has_upstream =
        std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
            return r.name == "upstream";
        });
    EXPECT_TRUE(has_origin);
    EXPECT_TRUE(has_upstream);
}

TEST(SyncTests, CurrentBranchSwitchesToConfiguredBranch) {
    int result = run_scenario("branch_switching");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_branch_switching");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("spoon-knife", repos[0].name);

    auto current_branch_it =
        std::ranges::find_if(repos[0].branches, [](const Branch &b) {
            return b.is_current;
        });
    ASSERT_NE(current_branch_it, repos[0].branches.end());
    EXPECT_EQ("main", current_branch_it->name);
    EXPECT_EQ("origin", current_branch_it->remote);

    bool has_main = std::ranges::any_of(repos[0].branches, [](const Branch &b) {
        return b.name == "main";
    });
    EXPECT_TRUE(has_main);
}

TEST(SyncTests, OutputFormattingWorksInNonTerminalMode) {
    if (detect_output_mode() != OutputMode::TEXT) {
        GTEST_SKIP() << "Only valid for non-terminal execution mode";
    }

    testing::internal::CaptureStdout();
    int result = run_scenario("basic_sync");
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, result);
    EXPECT_NE(std::string::npos, output.find("mrm report"))
        << "Missing report header in output";
    EXPECT_NE(std::string::npos, output.find("SUCCEEDED"))
        << "Missing success indicators in output";
}

TEST(SyncTests, PruneRemotesRemovesExtraRemotes) {
    int result = run_scenario("pruning_test");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_pruning");
    ASSERT_EQ(1, repos.size());

    bool has_origin =
        std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
            return r.name == "origin";
        });
    EXPECT_TRUE(has_origin);

    result = run_scenario_with_pruning("pruning_test", true, false, false);
    EXPECT_EQ(0, result);

    repos = find_repos("test_pruning");
    ASSERT_EQ(1, repos.size());

    has_origin = std::ranges::any_of(repos[0].remotes, [](const Remote &r) {
        return r.name == "origin";
    });
    EXPECT_TRUE(has_origin);
}

TEST(SyncTests, PruneBranchesRemovesNonConfiguredBranches) {
    int result = run_scenario("pruning_test");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos = find_repos("test_pruning");
    ASSERT_EQ(1, repos.size());

    bool has_master =
        std::ranges::any_of(repos[0].branches, [](const Branch &b) {
            return b.name == "master";
        });
    EXPECT_TRUE(has_master);

    result = run_scenario_with_pruning("pruning_test", false, true, false);
    EXPECT_EQ(0, result);

    repos = find_repos("test_pruning");
    ASSERT_EQ(1, repos.size());

    has_master = std::ranges::any_of(repos[0].branches, [](const Branch &b) {
        return b.name == "master";
    });
    EXPECT_TRUE(has_master);
}

TEST(SyncTests, PruneReposRemovesUntrackedRepositories) {
    int result = run_scenario("pruning_test");
    EXPECT_EQ(0, result);

    std::vector<Repo> repos_before = find_repos("test_pruning");
    ASSERT_EQ(1, repos_before.size());
    EXPECT_EQ("hello-world", repos_before[0].name);

    result = run_scenario_with_pruning("pruning_test", false, false, true);
    EXPECT_EQ(0, result);

    std::vector<Repo> repos_after = find_repos("test_pruning");
    ASSERT_EQ(1, repos_after.size());
    EXPECT_EQ("hello-world", repos_after[0].name);
}

TEST(SyncTests, TimeoutConfigurationIsRespected) {
    int result = run_sync(
        SyncOptions{
            .config_file = get_scenario_path("basic_sync"),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1,
            .timeout_seconds = 120});
    EXPECT_EQ(0, result);
}

TEST(SyncTests, ZeroTimeoutDisablesTimeout) {
    int result = run_sync(
        SyncOptions{
            .config_file = get_scenario_path("basic_sync"),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1,
            .timeout_seconds = 0});
    EXPECT_EQ(0, result);
}

TEST(SyncTests, DefaultTimeoutIsApplied) {
    int result = run_sync(
        SyncOptions{
            .config_file = get_scenario_path("basic_sync"),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 1});
    EXPECT_EQ(0, result);
}