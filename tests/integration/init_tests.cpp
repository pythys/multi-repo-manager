#include "command/init.hpp"
#include "test_utils.hpp"
#include "vcs/git_guard.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;
const GitGuard git_guard;

using test_utils::contains;
using test_utils::read_file;
using test_utils::ScopedTempCwd;

TEST(InitTests, CreatesBasicWorkspaceStructure) {
    ScopedTempCwd scratch("mrm-init-tests");

    InitOptions options;
    options.repos_path = "repos";

    ASSERT_EQ(0, run_init(options));

    EXPECT_TRUE(fs::exists("README.md"));
    EXPECT_TRUE(fs::exists("mrm.yml"));
    EXPECT_TRUE(fs::exists(".gitignore"));
    EXPECT_TRUE(fs::exists(".git"));
    EXPECT_TRUE(fs::is_directory("repos"));
}

TEST(InitTests, CustomReposPathSubstitutedInAllFiles) {
    ScopedTempCwd scratch("mrm-init-tests");

    InitOptions options;
    options.repos_path = "custom-repos";

    ASSERT_EQ(0, run_init(options));

    EXPECT_TRUE(fs::is_directory("custom-repos"));

    const std::string readme = read_file("README.md");
    EXPECT_TRUE(contains(readme, "custom-repos"));

    const std::string config = read_file("mrm.yml");
    EXPECT_TRUE(contains(config, "custom-repos"));

    const std::string gitignore = read_file(".gitignore");
    EXPECT_TRUE(contains(gitignore, "custom-repos/"));
}

TEST(InitTests, FailsIfDirectoryNotEmpty) {
    ScopedTempCwd scratch("mrm-init-tests");

    std::ofstream existing("some_file.txt");
    existing << "content\n";
    existing.close();

    InitOptions options;
    options.repos_path = "repos";

    EXPECT_EQ(1, run_init(options));
}
