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
