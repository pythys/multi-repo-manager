#include "test_utils.hpp"
#include "util/constants.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <random>

namespace fs = std::filesystem;
using test_utils::TempDir;

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

TEST_F(GitManagerTimeoutTests, CloneSucceedsAcrossTimeoutSettings) {
    const fs::path source = dir() / "source";
    GitManager::init(source.string(), "master");
    test_utils::write_file(source / "file.txt", "hi\n");
    GitManager::commit(source.string(), "init");

    for (const int timeout : {0, 120, DEFAULT_TIMEOUT}) {
        const fs::path clone = dir() / ("clone-" + std::to_string(timeout));
        EXPECT_NO_THROW(
            GitManager::clone(source.string(), clone.string(), timeout));
        EXPECT_TRUE(fs::exists(clone / "file.txt"));
    }
}

TEST(GitManagerLocalTests, CommitProducesCloneableHistory) {
    TempDir temp;
    const fs::path repo = temp.path() / "src";
    GitManager::init(repo.string(), "master");
    test_utils::write_file(repo / "file.txt", "hello\n");
    GitManager::commit(repo.string(), "initial");

    EXPECT_FALSE(GitManager::get_status(repo.string()).has_changes);
    EXPECT_TRUE(GitManager::branch_exists(repo.string(), "master"));

    const fs::path clone = temp.path() / "clone";
    GitManager::clone(repo.string(), clone.string());
    EXPECT_TRUE(fs::exists(clone / "file.txt"));
}

TEST(GitManagerLocalTests, CommitSupportsMultipleCommits) {
    TempDir temp;
    const fs::path repo = temp.path() / "src";
    GitManager::init(repo.string(), "master");
    test_utils::write_file(repo / "a.txt", "a\n");
    GitManager::commit(repo.string(), "first");
    test_utils::write_file(repo / "b.txt", "b\n");
    GitManager::commit(repo.string(), "second");

    EXPECT_FALSE(GitManager::get_status(repo.string()).has_changes);
}

TEST(GitManagerLocalTests, CreateBranchAtHead) {
    TempDir temp;
    const fs::path repo = temp.path() / "src";
    GitManager::init(repo.string(), "master");
    test_utils::write_file(repo / "file.txt", "hello\n");
    GitManager::commit(repo.string(), "initial");

    EXPECT_FALSE(GitManager::branch_exists(repo.string(), "feature"));
    GitManager::create_branch(repo.string(), "feature");
    EXPECT_TRUE(GitManager::branch_exists(repo.string(), "feature"));
}

TEST(GitManagerLocalTests, CreateBranchIsCheckoutableViaClone) {
    TempDir temp;
    const fs::path repo = temp.path() / "src";
    GitManager::init(repo.string(), "master");
    test_utils::write_file(repo / "file.txt", "hello\n");
    GitManager::commit(repo.string(), "initial");
    GitManager::create_branch(repo.string(), "feature");

    const fs::path clone = temp.path() / "clone";
    GitManager::clone(
        repo.string(),
        clone.string(),
        DEFAULT_TIMEOUT,
        0,
        "feature");
    EXPECT_TRUE(fs::exists(clone / "file.txt"));
}

TEST(GitManagerLocalTests, GetBranchesReturnsDeterministicSortedOrder) {
    TempDir temp;
    const fs::path source = temp.path() / "source";
    GitManager::init(source.string(), "master");
    test_utils::write_file(source / "file.txt", "hello\n");
    GitManager::commit(source.string(), "initial");
    GitManager::create_branch(source.string(), "agentic");

    const fs::path clone = temp.path() / "clone";
    GitManager::clone(source.string(), clone.string());
    GitManager::add_branch(
        clone.string(),
        Branch{.name = "agentic", .remote = "origin", .is_current = false});

    const std::vector<Branch> branches =
        GitManager::get_branches(clone.string());
    ASSERT_EQ(2, branches.size());
    EXPECT_EQ("agentic", branches[0].name);
    EXPECT_FALSE(branches[0].is_current);
    EXPECT_EQ("master", branches[1].name);
    EXPECT_TRUE(branches[1].is_current);
}

TEST(GitManagerLocalTests, GetRemotesReturnsDeterministicSortedOrder) {
    TempDir temp;
    const fs::path repo = temp.path() / "src";
    GitManager::init(repo.string(), "master");
    GitManager::add_remote(
        repo.string(),
        Remote{.name = "upstream", .url = "https://example.com/upstream.git"});
    GitManager::add_remote(
        repo.string(),
        Remote{.name = "agentic", .url = "https://example.com/agentic.git"});

    const std::vector<Remote> remotes = GitManager::get_remotes(repo.string());
    ASSERT_EQ(2, remotes.size());
    EXPECT_EQ("agentic", remotes[0].name);
    EXPECT_EQ("upstream", remotes[1].name);
}

TEST(GitManagerNetworkTests, CloneFromGitHubOverHttps) {
    TempDir temp;
    const fs::path clone = temp.path() / "hello";
    EXPECT_NO_THROW(
        GitManager::clone(
            "https://github.com/octocat/Hello-World",
            clone.string()));
    EXPECT_TRUE(fs::exists(clone / "README"));
}
