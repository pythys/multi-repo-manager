#include "core/tree.hpp"
#include "git_test_utils.hpp"
#include "test_utils.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/repo_factory.hpp"
#include <algorithm>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
const GitGuard git_guard;
using test_utils::TempDir;
using test_utils::write_file;

bool contains_status(
    const std::vector<std::string> &statuses,
    const std::string &needle) {
    return std::ranges::any_of(statuses, [&](const std::string &line) {
        return line.find(needle) != std::string::npos;
    });
}
} // namespace

TEST(StatusTests, ReportsRenameIgnoredAndUntracked) {
    TempDir temp;
    const fs::path &repo = temp.path();
    git_test::init_repo(repo, "master");
    git_test::set_user(repo, "test@example.com", "test");

    write_file(repo / ".gitignore", "*.log\n");
    write_file(repo / "rename_idx.txt", "rename index\n");
    write_file(repo / "rename_wt.txt", "rename worktree\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "initial");

    fs::rename(repo / "rename_idx.txt", repo / "rename_idx_new.txt");
    git_test::stage_rename(repo, "rename_idx.txt", "rename_idx_new.txt");
    fs::rename(repo / "rename_wt.txt", repo / "rename_wt_new.txt");
    write_file(repo / "ignored.log", "ignored\n");
    write_file(repo / "untracked.txt", "untracked\n");

    auto repo_manager = create_repo_manager(RepoType::GIT);
    const RepoStatus status = repo_manager->get_status(repo.string());

    EXPECT_TRUE(status.has_changes);
    EXPECT_TRUE(contains_status(status.messages, "Renamed file staged"));
    EXPECT_TRUE(
        contains_status(status.messages, "Renamed file: rename_wt_new.txt"));
    EXPECT_FALSE(contains_status(status.messages, "ignored.log"));
    EXPECT_TRUE(contains_status(status.messages, "New file: untracked.txt"));
}

TEST(StatusTests, ReportsConflictedFiles) {
    TempDir temp;
    const fs::path &repo = temp.path();
    git_test::init_repo(repo, "master");
    git_test::set_user(repo, "test@example.com", "test");

    write_file(repo / "conflict.txt", "base\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "base");

    git_test::create_and_checkout_branch(repo, "feature");
    write_file(repo / "conflict.txt", "feature\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "feature");

    git_test::checkout_branch(repo, "master");
    write_file(repo / "conflict.txt", "master\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "master");
    EXPECT_FALSE(git_test::merge_branch(repo, "feature"));

    auto repo_manager = create_repo_manager(RepoType::GIT);
    const RepoStatus status = repo_manager->get_status(repo.string());
    EXPECT_TRUE(status.has_changes);
    EXPECT_TRUE(
        contains_status(status.messages, "Conflicted file: conflict.txt"));
}
