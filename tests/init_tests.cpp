#include "git_guard.hpp"
#include "init.hpp"
#include "test_utils.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;
const GitGuard git_guard;

using test_utils::contains;
using test_utils::CurrentPathGuard;
using test_utils::read_file;
using test_utils::TempDir;

TEST(InitTests, CreatesBasicWorkspaceStructure) {
    TempDir temp("mrm-init-tests");
    CurrentPathGuard cwd_guard;
    fs::current_path(temp.path());

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
    TempDir temp("mrm-init-tests");
    CurrentPathGuard cwd_guard;
    fs::current_path(temp.path());

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
    TempDir temp("mrm-init-tests");
    CurrentPathGuard cwd_guard;
    fs::current_path(temp.path());

    std::ofstream existing("some_file.txt");
    existing << "content\n";
    existing.close();

    InitOptions options;
    options.repos_path = "repos";

    EXPECT_EQ(1, run_init(options));
}
