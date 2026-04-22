#include "command/exec.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
std::vector<Tree> sample_config() {
    return {
        Tree{
            .root = "/workspace-a",
            .repos =
                {
                    Repo{
                        .name = "repo1",

                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                    Repo{
                        .name = "repo2",

                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                }},
        Tree{
            .root = "/workspace-b",
            .repos =
                {
                    Repo{
                        .name = "repo3",

                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                }},
    };
}
} // namespace

TEST(ExecTests, PlanIncludesMatchingReposWithResolvedPaths) {
    const ExecPlanResult plan = plan_exec("git status -sb", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ("/workspace-a/repo1", plan.items[0].repo_path);
    EXPECT_EQ("/workspace-a/repo2", plan.items[1].repo_path);
    EXPECT_EQ("/workspace-b/repo3", plan.items[2].repo_path);
}

TEST(ExecTests, CommandPartsPreserveStructure) {
    const ExecPlanResult plan =
        plan_exec("git rev-parse --is-inside-work-tree", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_FALSE(plan.items.empty());
    EXPECT_EQ(std::string("git"), plan.items[0].command_parts[0]);
    ASSERT_GE(plan.items[0].command_parts.size(), 2);
    EXPECT_EQ(std::string("rev-parse"), plan.items[0].command_parts[1]);
}

TEST(ExecTests, InvalidCommandSyntaxReturnsPlanError) {
    const ExecPlanResult plan =
        plan_exec("git rev-parse \"oops", sample_config());
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, PlaceholderPathSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo"), plan.items[0].command_parts[0]);
    EXPECT_EQ(
        std::string("/workspace-a/repo1"),
        plan.items[0].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[1].command_parts[0]);
    EXPECT_EQ(
        std::string("/workspace-a/repo2"),
        plan.items[1].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[2].command_parts[0]);
    EXPECT_EQ(
        std::string("/workspace-b/repo3"),
        plan.items[2].command_parts[1]);
}

TEST(ExecTests, PlaceholderNameSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {name}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo"), plan.items[0].command_parts[0]);
    EXPECT_EQ(std::string("repo1"), plan.items[0].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[1].command_parts[0]);
    EXPECT_EQ(std::string("repo2"), plan.items[1].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[2].command_parts[0]);
    EXPECT_EQ(std::string("repo3"), plan.items[2].command_parts[1]);
}

TEST(ExecTests, PlaceholderRootSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {root}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo"), plan.items[0].command_parts[0]);
    EXPECT_EQ(std::string("/workspace-a"), plan.items[0].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[1].command_parts[0]);
    EXPECT_EQ(std::string("/workspace-a"), plan.items[1].command_parts[1]);
    EXPECT_EQ(std::string("echo"), plan.items[2].command_parts[0]);
    EXPECT_EQ(std::string("/workspace-b"), plan.items[2].command_parts[1]);
}

TEST(ExecTests, MultiplePlaceholdersInSameCommand) {
    const ExecPlanResult plan =
        plan_exec("echo {name} at {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo"), plan.items[0].command_parts[0]);
    EXPECT_EQ(std::string("repo1"), plan.items[0].command_parts[1]);
    EXPECT_EQ(std::string("at"), plan.items[0].command_parts[2]);
    EXPECT_EQ(
        std::string("/workspace-a/repo1"),
        plan.items[0].command_parts[3]);
}

TEST(ExecTests, PlaceholderInMiddleOfArgument) {
    const ExecPlanResult plan =
        plan_exec("tar -czf /backup/{name}.tar.gz {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("tar"), plan.items[0].command_parts[0]);
    EXPECT_EQ(std::string("-czf"), plan.items[0].command_parts[1]);
    EXPECT_EQ(
        std::string("/backup/repo1.tar.gz"),
        plan.items[0].command_parts[2]);
    EXPECT_EQ(
        std::string("/workspace-a/repo1"),
        plan.items[0].command_parts[3]);
}

TEST(ExecTests, CommandWithoutPlaceholders) {
    const ExecPlanResult plan = plan_exec("ls -la", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("ls"), plan.items[0].command_parts[0]);
    EXPECT_EQ(std::string("-la"), plan.items[0].command_parts[1]);
}
