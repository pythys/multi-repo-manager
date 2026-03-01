#include "exec.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
std::vector<Tree> sample_config() {
    return {
        Tree{
            .root = "workspace-a",
            .repos =
                {
                    Repo{
                        .name = "repo1",
                        .type = RepoType::GIT,
                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                    Repo{
                        .name = "repo2",
                        .type = RepoType::SVN,
                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                }},
        Tree{
            .root = "workspace-b",
            .repos =
                {
                    Repo{
                        .name = "repo3",
                        .type = RepoType::GIT,
                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages = {}},
                }},
    };
}
} // namespace

TEST(ExecTests, PlanIncludesMatchingReposWithResolvedPaths) {
    const ExecPlanResult plan =
        plan_exec("git status -sb", sample_config(), "git");

    ASSERT_TRUE(plan.error.empty());
    ASSERT_EQ(2, plan.items.size());
    EXPECT_EQ("workspace-a/repo1", plan.items[0].repo_path);
    EXPECT_EQ("workspace-b/repo3", plan.items[1].repo_path);
}

TEST(ExecTests, PlanKeepsCommandWhenAlreadyPrefixed) {
    const ExecPlanResult plan = plan_exec(
        "git rev-parse --is-inside-work-tree",
        sample_config(),
        "git");

    ASSERT_TRUE(plan.error.empty());
    ASSERT_FALSE(plan.items.empty());
    EXPECT_EQ(std::string("git"), plan.items[0].command_parts[0]);
    ASSERT_GE(plan.items[0].command_parts.size(), 2);
    EXPECT_EQ(std::string("rev-parse"), plan.items[0].command_parts[1]);
}

TEST(ExecTests, InvalidRepoTypeReturnsPlanError) {
    const ExecPlanResult plan = plan_exec("status", sample_config(), "invalid");
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, InvalidCommandSyntaxReturnsPlanError) {
    const ExecPlanResult plan =
        plan_exec("git rev-parse \"oops", sample_config(), "git");
    EXPECT_FALSE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}

TEST(ExecTests, RepoTypeFilterCanReturnEmptyPlanWithoutError) {
    const ExecPlanResult plan = plan_exec("git status", sample_config(), "hg");
    EXPECT_TRUE(plan.error.empty());
    EXPECT_TRUE(plan.items.empty());
}
