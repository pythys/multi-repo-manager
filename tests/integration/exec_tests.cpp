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

TEST(ExecTests, CommandPreservesStructure) {
    const ExecPlanResult plan =
        plan_exec("git rev-parse --is-inside-work-tree", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_FALSE(plan.items.empty());
    EXPECT_EQ(
        std::string("git rev-parse --is-inside-work-tree"),
        plan.items[0].command);
}

TEST(ExecTests, CommandPreservesQuotedArguments) {
    const ExecPlanResult plan =
        plan_exec("git commit -m \"hello world\"", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_FALSE(plan.items.empty());
    EXPECT_EQ(
        std::string("git commit -m \"hello world\""),
        plan.items[0].command);
}

TEST(ExecTests, CommandPreservesSingleQuotedArguments) {
    const ExecPlanResult plan =
        plan_exec("find {path} -name '*.cpp'", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_FALSE(plan.items.empty());
    EXPECT_EQ(
        std::string("find /workspace-a/repo1 -name '*.cpp'"),
        plan.items[0].command);
}

TEST(ExecTests, InvalidCommandSyntaxReturnsPlanError) {
    const ExecPlanResult plan =
        plan_exec("git rev-parse \"oops", sample_config());
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, UnterminatedSingleQuoteReturnsPlanError) {
    const ExecPlanResult plan = plan_exec("echo 'oops", sample_config());
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, TrailingEscapeReturnsPlanError) {
    const ExecPlanResult plan = plan_exec("echo foo\\", sample_config());
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, PlaceholderPathSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo /workspace-a/repo1"), plan.items[0].command);
    EXPECT_EQ(std::string("echo /workspace-a/repo2"), plan.items[1].command);
    EXPECT_EQ(std::string("echo /workspace-b/repo3"), plan.items[2].command);
}

TEST(ExecTests, PlaceholderNameSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {name}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo repo1"), plan.items[0].command);
    EXPECT_EQ(std::string("echo repo2"), plan.items[1].command);
    EXPECT_EQ(std::string("echo repo3"), plan.items[2].command);
}

TEST(ExecTests, PlaceholderRootSubstitution) {
    const ExecPlanResult plan = plan_exec("echo {root}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("echo /workspace-a"), plan.items[0].command);
    EXPECT_EQ(std::string("echo /workspace-a"), plan.items[1].command);
    EXPECT_EQ(std::string("echo /workspace-b"), plan.items[2].command);
}

TEST(ExecTests, MultiplePlaceholdersInSameCommand) {
    const ExecPlanResult plan =
        plan_exec("echo {name} at {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(
        std::string("echo repo1 at /workspace-a/repo1"),
        plan.items[0].command);
}

TEST(ExecTests, PlaceholderInMiddleOfArgument) {
    const ExecPlanResult plan =
        plan_exec("tar -czf /backup/{name}.tar.gz {path}", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(
        std::string("tar -czf /backup/repo1.tar.gz /workspace-a/repo1"),
        plan.items[0].command);
}

TEST(ExecTests, CommandWithoutPlaceholders) {
    const ExecPlanResult plan = plan_exec("ls -la", sample_config());

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(3, plan.items.size());
    EXPECT_EQ(std::string("ls -la"), plan.items[0].command);
}
