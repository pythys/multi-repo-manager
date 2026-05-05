#include "util/constants.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>

const GitGuard git_guard;

class GitManagerTimeoutTests : public ::testing::Test {
  protected:
    void SetUp() override {
        test_dir = std::filesystem::temp_directory_path() /
                   ("mrm_git_timeout_test_" + std::to_string(std::rand()));
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::filesystem::path test_dir;
};

TEST_F(GitManagerTimeoutTests, DefaultTimeoutConstantIsCorrect) {
    EXPECT_EQ(300, DEFAULT_TIMEOUT);
}

TEST_F(GitManagerTimeoutTests, CloneWithCustomTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = test_dir / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string(),
            120);
    });
}

TEST_F(GitManagerTimeoutTests, CloneWithZeroTimeoutDisablesTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = test_dir / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string(),
            0);
    });
}

TEST_F(GitManagerTimeoutTests, CloneWithDefaultTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = test_dir / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string());
    });
}

TEST_F(GitManagerTimeoutTests, DISABLED_TimeoutActuallyTriggersOnSlowConnection) {
    GitManager manager;
    std::filesystem::path repo_path = test_dir / "timeout_repo";

    auto start = std::chrono::steady_clock::now();

    EXPECT_THROW(
        {
            manager.clone(
                "http://10.255.255.1/repo.git",
                repo_path.string(),
                5);
        },
        std::runtime_error);

    auto end = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::seconds>(end - start);

    EXPECT_GE(duration.count(), 4);
    EXPECT_LT(duration.count(), 15);
}

TEST_F(GitManagerTimeoutTests, ZeroTimeoutAllowsSlowConnections) {
    GitManager manager;
    std::filesystem::path repo_path = test_dir / "no_timeout_repo";

    EXPECT_THROW(
        {
            manager.clone(
                "http://192.0.2.1/nonexistent.git",
                repo_path.string(),
                0);
        },
        std::runtime_error);
}