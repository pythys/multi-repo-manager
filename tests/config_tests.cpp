#include <gtest/gtest.h>
#include "config.hpp"

TEST(ConfigTests, FileParsing) {
  std::vector<Tree> trees = get_config(std::string(TEST_RESOURCES_DIR) + "/github_repos.yml");
  EXPECT_EQ(trees.size(), 1);
}
