#include <gtest/gtest.h>
#include "config.hpp"

std::vector<Tree> parse_config(const std::string& filename) {
    return get_config(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

TEST(ConfigTests, HttpsProtocolIdentified) {
    std::vector<Tree> trees = parse_config("git_protocols.yml");
    EXPECT_EQ(trees[0].repos[0].remotes[0].type, RemoteType::HTTPS);
}

TEST(ConfigTests, GitProtocolIdentified) {
    std::vector<Tree> trees = parse_config("git_protocols.yml");
    EXPECT_EQ(trees[1].repos[0].remotes[0].type, RemoteType::GIT);
}

TEST(ConfigTests, SshProtocolIdentified) {
    std::vector<Tree> trees = parse_config("git_protocols.yml");
    EXPECT_EQ(trees[2].repos[0].remotes[0].type, RemoteType::SSH);
}
