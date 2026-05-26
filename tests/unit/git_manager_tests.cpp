#include "util/constants.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <random>

const GitGuard git_guard;

class GitManagerTimeoutTests : public ::testing::Test {
  protected:
    void SetUp() override {
        std::random_device rng;
        test_dir = std::filesystem::temp_directory_path() /
                   ("mrm_git_timeout_test_" + std::to_string(rng()));
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

  private:
    std::filesystem::path test_dir;

  protected:
    [[nodiscard]] auto dir() const -> const std::filesystem::path & {
        return test_dir;
    }
};

TEST_F(GitManagerTimeoutTests, DefaultTimeoutConstantIsCorrect) {
    EXPECT_EQ(300, DEFAULT_TIMEOUT);
}

TEST_F(GitManagerTimeoutTests, CloneWithCustomTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = dir() / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string(),
            120);
    });
}

TEST_F(GitManagerTimeoutTests, CloneWithZeroTimeoutDisablesTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = dir() / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string(),
            0);
    });
}

TEST_F(GitManagerTimeoutTests, CloneWithDefaultTimeout) {
    GitManager manager;
    std::filesystem::path repo_path = dir() / "test_repo";

    EXPECT_NO_THROW({
        manager.clone(
            "https://github.com/octocat/Hello-World.git",
            repo_path.string());
    });
}
