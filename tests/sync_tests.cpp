#include "config.hpp"
#include "find.hpp"
#include "git_guard.hpp"
#include "runtime.hpp"
#include "sync.hpp"
#include <gtest/gtest.h>
#include <ranges>
#include <string>
#include <vector>

const GitGuard git_guard;

int sync(const std::string &filename) {
    return run_sync(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

TEST(SyncTests, NestedSync) {
    sync("nested_repos.yml");
    std::vector<Repo> repos = find_repos("nested");
    EXPECT_EQ(4, repos.size());
    EXPECT_EQ("parent", repos[0].name);
    EXPECT_EQ("parent/child1", repos[1].name);
    EXPECT_EQ("parent/child1/not_a_repo/grandchild1", repos[2].name);
    EXPECT_EQ("parent/child2", repos[3].name);
}

TEST(SyncTests, MultipleTrees) {
    sync("multi_tree.yml");
    std::vector<Repo> first = find_repos("first_root");
    std::vector<Repo> second = find_repos("second_root");
    EXPECT_EQ(1, first.size());
    EXPECT_EQ(2, second.size());
    EXPECT_EQ(first[0].name, "dust");
    EXPECT_EQ(second[1].name, "st");
}

TEST(SyncTests, BranchSyncMatchesConfig) {
    const std::string config_file =
        std::string(TEST_RESOURCES_DIR) + "/branch_sync.yml";
    sync("branch_sync.yml");
    std::vector<Repo> repos = find_repos("branch_root");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("fzf", repos[0].name);

    std::vector<Tree> trees = get_config(config_file);
    ASSERT_EQ(trees.size(), 1);
    ASSERT_EQ(trees[0].repos.size(), 1);
    const Repo &expected = trees[0].repos[0];

    auto find_branch = [&](const std::vector<Branch> &branches,
                           const std::string &name) -> const Branch * {
        auto it = std::ranges::find_if(branches, [&](const Branch &branch) {
            return branch.name == name;
        });
        return it == branches.end() ? nullptr : &(*it);
    };

    for (const auto &expected_branch : expected.branches) {
        const Branch *actual =
            find_branch(repos[0].branches, expected_branch.name);
        ASSERT_NE(actual, nullptr);
        EXPECT_EQ(actual->remote, expected_branch.remote);
        EXPECT_EQ(actual->is_current, expected_branch.is_current);
    }
}

TEST(SyncTests, EmitsTextOutputWhenNotInTerminal) {
    if (detect_output_mode() != OutputMode::TEXT) {
        GTEST_SKIP() << "Only valid for non-terminal execution mode";
    }

    testing::internal::CaptureStdout();
    sync("nested_repos.yml");
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(std::string::npos, out.find("[nested/parent]"));
    EXPECT_NE(std::string::npos, out.find("RUNNING"));
    EXPECT_NE(std::string::npos, out.find("SUCCEEDED"));
}
