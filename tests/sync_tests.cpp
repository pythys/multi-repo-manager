#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "find.hpp"
#include "git_guard.hpp"
#include "sync.hpp"

const GitGuard git_guard;

int sync(const std::string& filename) {
    return run_sync(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

TEST(SyncTests, NestedSync) {
    sync("nested_repos.yml");
    std::vector<Repo> repos = find_repos("nested");
    EXPECT_EQ(4, repos.size());
    EXPECT_EQ("parent", repos[0].name);
    EXPECT_EQ("parent/child1", repos[1].name);
    EXPECT_EQ("parent/child1/grandchild1", repos[2].name);
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

TEST(SyncTests, MultipleNests) {
    sync("multi_nests.yml");
    std::vector<Repo> repos = find_repos("multinested");
    EXPECT_EQ(8, repos.size());
    EXPECT_EQ(repos[0].name, "parent1");
    EXPECT_EQ(repos[1].name, "parent1/child1");
    EXPECT_EQ(repos[2].name, "parent1/child1/grandchild1");
    EXPECT_EQ(repos[3].name, "parent1/child2");
    EXPECT_EQ(repos[4].name, "parent2");
    EXPECT_EQ(repos[5].name, "parent2/child1");
    EXPECT_EQ(repos[6].name, "parent2/child1/grandchild1");
    EXPECT_EQ(repos[7].name, "parent2/child2");
}
