#include "config.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

std::vector<Tree> parse_config(const std::string &filename) {
    return get_config(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

std::vector<Tree> parse_dependencies(const std::string &filename) {
    return get_dependencies(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

TEST(ConfigTests, WithoutNesting) {
    std::vector<Tree> trees = parse_config("nested_repos.yml");
    EXPECT_EQ(trees[0].repos.size(), 4);
}

TEST(ConfigTests, NestingFirstLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repos.yml");
    EXPECT_EQ(trees[0].repos.size(), 1);
}

TEST(ConfigTests, NestingSecondLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repos.yml");
    EXPECT_EQ(trees[0].repos[0].children.size(), 2);
}

TEST(ConfigTests, NestingThirdLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repos.yml");
    EXPECT_EQ(trees[0].repos[0].children[0].children.size(), 1);
    EXPECT_EQ(trees[0].repos[0].children[1].children.size(), 0);
}

TEST(ConfigTests, MultipleTrees) {
    std::vector<Tree> trees = parse_dependencies("multi_tree.yml");
    EXPECT_EQ(trees.size(), 2);
}
