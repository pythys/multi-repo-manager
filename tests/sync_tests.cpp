#include "config.hpp"
#include "find.hpp"
#include "git_guard.hpp"
#include "runtime.hpp"
#include "sync.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ranges>
#include <string>
#include <vector>

namespace fs = std::filesystem;

const GitGuard git_guard;

int sync(const std::string &filename) {
    return run_sync(std::string(TEST_RESOURCES_DIR) + "/" + filename);
}

int sync(const std::string &filename, bool prune_remotes, bool prune_branches) {
    return run_sync(
        std::string(TEST_RESOURCES_DIR) + "/" + filename,
        0,
        prune_remotes,
        prune_branches);
}

namespace {
class TempDir {
  public:
    TempDir() {
        const auto unique = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = fs::temp_directory_path() / ("mrm-sync-tests-" + unique);
        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path &path() const {
        return path_;
    }

  private:
    fs::path path_;
};

int run_git(const fs::path &repo, const std::string &command) {
    const std::string cmd =
        "git -C \"" + repo.string() + "\" " + command + " >/dev/null 2>&1";
    return std::system(cmd.c_str());
}

void write_text(const fs::path &path, const std::string &content) {
    std::ofstream out(path);
    out << content;
}

void write_main_only_config(
    const fs::path &path,
    const fs::path &root,
    const fs::path &origin_remote) {
    std::ofstream out(path);
    out << "trees:\n";
    out << "- root: " << root.string() << "\n";
    out << "  repos:\n";
    out << "  - name: repo1\n";
    out << "    type: git\n";
    out << "    remotes:\n";
    out << "    - name: origin\n";
    out << "      url: " << origin_remote.string() << "\n";
    out << "    branches:\n";
    out << "    - name: main\n";
    out << "      remote: origin\n";
    out << "      is_current: true\n";
}

bool has_remote(const Repo &repo, const std::string &name) {
    auto it = std::ranges::find_if(repo.remotes, [&](const Remote &remote) {
        return remote.name == name;
    });
    return it != repo.remotes.end();
}

const Branch *find_branch(const Repo &repo, const std::string &name) {
    auto it = std::ranges::find_if(repo.branches, [&](const Branch &branch) {
        return branch.name == name;
    });
    return it == repo.branches.end() ? nullptr : &(*it);
}
} // namespace

TEST(SyncTests, NestedSync) {
    sync("nested_repos.yml");
    std::vector<Repo> repos = find_repos("nested");
    EXPECT_EQ(4, repos.size());
    EXPECT_EQ("parent", repos[0].name);
    EXPECT_EQ("parent/child1", repos[1].name);
    EXPECT_EQ("parent/child1/not_a_repo/grandchild1", repos[2].name);
    EXPECT_EQ("parent/child2", repos[3].name);
}

TEST(SyncTests, MultipleTrees) {
    sync("multi_tree.yml");
    std::vector<Repo> first = find_repos("first_root");
    std::vector<Repo> second = find_repos("second_root");
    EXPECT_EQ(1, first.size());
    EXPECT_EQ(2, second.size());
    EXPECT_EQ(first[0].name, "dust");
    EXPECT_EQ(second[1].name, "st");
}

TEST(SyncTests, BranchSyncMatchesConfig) {
    const std::string config_file =
        std::string(TEST_RESOURCES_DIR) + "/branch_sync.yml";
    sync("branch_sync.yml");
    std::vector<Repo> repos = find_repos("branch_root");
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("fzf", repos[0].name);

    std::vector<Tree> trees = get_config(config_file);
    ASSERT_EQ(trees.size(), 1);
    ASSERT_EQ(trees[0].repos.size(), 1);
    const Repo &expected = trees[0].repos[0];

    auto find_branch = [&](const std::vector<Branch> &branches,
                           const std::string &name) -> const Branch * {
        auto it = std::ranges::find_if(branches, [&](const Branch &branch) {
            return branch.name == name;
        });
        return it == branches.end() ? nullptr : &(*it);
    };

    for (const auto &expected_branch : expected.branches) {
        const Branch *actual =
            find_branch(repos[0].branches, expected_branch.name);
        ASSERT_NE(actual, nullptr);
        EXPECT_EQ(actual->remote, expected_branch.remote);
        EXPECT_EQ(actual->is_current, expected_branch.is_current);
    }
}

TEST(SyncTests, EmitsTextOutputWhenNotInTerminal) {
    if (detect_output_mode() != OutputMode::TEXT) {
        GTEST_SKIP() << "Only valid for non-terminal execution mode";
    }

    testing::internal::CaptureStdout();
    sync("nested_repos.yml");
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(std::string::npos, out.find("[nested/parent]"));
    EXPECT_NE(std::string::npos, out.find("RUNNING"));
    EXPECT_NE(std::string::npos, out.find("SUCCEEDED"));
}

TEST(SyncTests, ExistingBranchesDoNotRequireFetch) {
    if (detect_output_mode() != OutputMode::TEXT) {
        GTEST_SKIP() << "Only valid for non-terminal execution mode";
    }

    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path remote = temp.path() / "remote.git";
    const fs::path invalid_remote = temp.path() / "missing-remote.git";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(temp.path(), "init --bare remote.git"));
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));

    write_text(repo / "README.md", "hello\n");
    ASSERT_EQ(0, run_git(repo, "add README.md"));
    ASSERT_EQ(0, run_git(repo, "commit -m init"));
    ASSERT_EQ(
        0,
        run_git(repo, "remote add origin \"" + remote.string() + "\""));
    ASSERT_EQ(0, run_git(repo, "push -u origin main"));
    ASSERT_EQ(
        0,
        run_git(
            repo,
            "remote set-url origin \"" + invalid_remote.string() + "\""));

    const fs::path config = temp.path() / "config.yml";
    std::ofstream out(config);
    out << "trees:\n";
    out << "- root: " << root.string() << "\n";
    out << "  repos:\n";
    out << "  - name: repo1\n";
    out << "    type: git\n";
    out << "    remotes:\n";
    out << "    - name: origin\n";
    out << "      url: " << invalid_remote.string() << "\n";
    out << "    branches:\n";
    out << "    - name: main\n";
    out << "      remote: origin\n";
    out << "      is_current: true\n";
    out.close();

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    run_sync(config.string());
    const std::string stdout_text = testing::internal::GetCapturedStdout();
    const std::string stderr_text = testing::internal::GetCapturedStderr();

    EXPECT_NE(std::string::npos, stdout_text.find("SUCCEEDED"));
    EXPECT_EQ(std::string::npos, stdout_text.find("FAILED"));
    EXPECT_EQ(std::string::npos, stderr_text.find("FAILED"));
}

TEST(SyncTests, PruneRemotesIsOptIn) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path remote = temp.path() / "remote.git";
    const fs::path upstream = temp.path() / "upstream.git";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(temp.path(), "init --bare remote.git"));
    ASSERT_EQ(0, run_git(temp.path(), "init --bare upstream.git"));
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));
    write_text(repo / "README.md", "hello\n");
    ASSERT_EQ(0, run_git(repo, "add README.md"));
    ASSERT_EQ(0, run_git(repo, "commit -m init"));
    ASSERT_EQ(
        0,
        run_git(repo, "remote add origin \"" + remote.string() + "\""));
    ASSERT_EQ(0, run_git(repo, "push -u origin main"));
    ASSERT_EQ(
        0,
        run_git(repo, "remote add upstream \"" + upstream.string() + "\""));

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(config.string());
    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_TRUE(has_remote(repos[0], "upstream"));

    run_sync(config.string(), 0, true, false);
    repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_FALSE(has_remote(repos[0], "upstream"));
}

TEST(SyncTests, PruneBranchesRemovesNonCurrentBranches) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path remote = temp.path() / "remote.git";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(temp.path(), "init --bare remote.git"));
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));
    write_text(repo / "README.md", "hello\n");
    ASSERT_EQ(0, run_git(repo, "add README.md"));
    ASSERT_EQ(0, run_git(repo, "commit -m init"));
    ASSERT_EQ(
        0,
        run_git(repo, "remote add origin \"" + remote.string() + "\""));
    ASSERT_EQ(0, run_git(repo, "push -u origin main"));
    ASSERT_EQ(0, run_git(repo, "checkout -b feature"));
    write_text(repo / "feature.txt", "feature\n");
    ASSERT_EQ(0, run_git(repo, "add feature.txt"));
    ASSERT_EQ(0, run_git(repo, "commit -m feature"));
    ASSERT_EQ(0, run_git(repo, "push -u origin feature"));
    ASSERT_EQ(0, run_git(repo, "checkout main"));

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(config.string(), 0, false, true);
    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_NE(find_branch(repos[0], "main"), nullptr);
    EXPECT_EQ(find_branch(repos[0], "feature"), nullptr);
}

TEST(SyncTests, PruneBranchesDoesNotDeleteCurrentBranch) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path remote = temp.path() / "remote.git";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(temp.path(), "init --bare remote.git"));
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));
    write_text(repo / "README.md", "hello\n");
    ASSERT_EQ(0, run_git(repo, "add README.md"));
    ASSERT_EQ(0, run_git(repo, "commit -m init"));
    ASSERT_EQ(
        0,
        run_git(repo, "remote add origin \"" + remote.string() + "\""));
    ASSERT_EQ(0, run_git(repo, "push -u origin main"));
    ASSERT_EQ(0, run_git(repo, "checkout -b feature"));
    write_text(repo / "feature.txt", "feature\n");
    ASSERT_EQ(0, run_git(repo, "add feature.txt"));
    ASSERT_EQ(0, run_git(repo, "commit -m feature"));
    ASSERT_EQ(0, run_git(repo, "push -u origin feature"));

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(config.string(), 0, false, true);
    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    const Branch *main = find_branch(repos[0], "main");
    const Branch *feature = find_branch(repos[0], "feature");
    ASSERT_NE(main, nullptr);
    ASSERT_NE(feature, nullptr);
    EXPECT_TRUE(main->is_current);
    EXPECT_FALSE(feature->is_current);
}
