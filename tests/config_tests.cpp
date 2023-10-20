#include <gtest/gtest.h>
#include "config.hpp"

std::vector<Tree> parse_config(const std::string& filename) {
    return get_config(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

TEST(ConfigTests, TreesRecognition) {
    std::vector<Tree> trees = parse_config("git_protocols.yml");
    EXPECT_EQ(trees.size(), 3);
}
