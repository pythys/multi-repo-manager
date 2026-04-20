#include "command/sync.hpp"
#include "git_test_utils.hpp"
#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "util/command_options.hpp"
#include "util/runtime.hpp"
#include "vcs/git_guard.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ranges>
#include <string>
#include <vector>

namespace fs = std::filesystem;

const GitGuard git_guard;

int sync(const std::string &filename) {
    return run_sync(
        SyncOptions{
            .config_file = std::string(TEST_RESOURCES_DIR) + "/" + filename,
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 0,
        });
}

int sync(const std::string &filename, bool prune_remotes, bool prune_branches) {
    return run_sync(
        SyncOptions{
            .config_file = std::string(TEST_RESOURCES_DIR) + "/" + filename,
            .root_patterns = {},
            .prune_remotes = prune_remotes,
            .prune_branches = prune_branches,
            .prune_repos = false,
            .jobs = 0,
        });
}

namespace {
using test_utils::TempDir;
using test_utils::write_file;

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
    }
}

TEST(SyncTests, LeavesUpstreamUnchangedWhenMatches) {
    TempDir temp;
    const fs::path root = temp.path() / "repos";
    const fs::path repo = root / "repo1";
    const fs::path origin_remote = temp.path() / "origin.git";

    fs::create_directories(root);

    git_test::init_repo(origin_remote, "main", true);

    git_test::init_repo(repo, "main");
    git_test::set_user(repo, "test@example.com", "test");
    write_file(repo / "README.md", "test\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", origin_remote);
    git_test::push(repo, "origin", "main");
    git_test::set_upstream(repo, "main", "origin/main");

    auto branches_before = git_test::get_branches(repo);
    ASSERT_EQ(1, branches_before.size());
    EXPECT_EQ("origin", branches_before[0].remote);

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, origin_remote);

    int result = run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 0,
        });

    EXPECT_EQ(0, result);

    auto branches_after = git_test::get_branches(repo);
    ASSERT_EQ(1, branches_after.size());
    EXPECT_EQ("main", branches_after[0].name);
    EXPECT_EQ("origin", branches_after[0].remote);
}

TEST(SyncTests, ChangesUpstreamWhenNotMatching) {
    TempDir temp;
    const fs::path root = temp.path() / "repos";
    const fs::path repo = root / "repo1";
    const fs::path origin_remote = temp.path() / "origin.git";
    const fs::path upstream_remote = temp.path() / "upstream.git";

    fs::create_directories(root);

    git_test::init_repo(origin_remote, "master", true);
    git_test::init_repo(upstream_remote, "master", true);

    git_test::init_repo(repo, "master");
    git_test::set_user(repo, "test@example.com", "test");
    write_file(repo / "README.md", "test\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", origin_remote);
    git_test::add_remote(repo, "upstream", upstream_remote);
    git_test::push(repo, "origin", "master");
    git_test::push(repo, "upstream", "master");
    git_test::set_upstream(repo, "master", "origin/master");

    auto branches_before = git_test::get_branches(repo);
    ASSERT_EQ(1, branches_before.size());
    EXPECT_EQ("origin", branches_before[0].remote);

    const fs::path config = temp.path() / "config.yml";
    write_file(
        config,
        "trees:\n"
        "  - root: " +
            root.string() +
            "\n"
            "    repos:\n"
            "      - name: repo1\n"
            "        type: git\n"
            "        remotes:\n"
            "          - name: origin\n"
            "            url: " +
            origin_remote.string() +
            "\n"
            "          - name: upstream\n"
            "            url: " +
            upstream_remote.string() +
            "\n"
            "        branches:\n"
            "          - name: master\n"
            "            remote: upstream\n"
            "            is_current: true\n");

    int result = run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 0,
        });

    EXPECT_EQ(0, result);

    auto branches_after = git_test::get_branches(repo);
    ASSERT_EQ(1, branches_after.size());
    EXPECT_EQ("master", branches_after[0].name);
    EXPECT_EQ("upstream", branches_after[0].remote);
}

TEST(SyncTests, SwitchesToCurrentBranchFromConfig) {
    TempDir temp;
    const fs::path root = temp.path() / "repos";
    const fs::path repo = root / "repo1";
    const fs::path origin_remote = temp.path() / "origin.git";

    fs::create_directories(root);

    git_test::init_repo(origin_remote, "master", true);

    git_test::init_repo(repo, "master");
    git_test::set_user(repo, "test@example.com", "test");
    write_file(repo / "README.md", "test\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", origin_remote);
    git_test::push(repo, "origin", "master");
    git_test::set_upstream(repo, "master", "origin/master");

    git_test::create_branch(repo, "develop");
    git_test::push(repo, "origin", "develop");
    git_test::set_upstream(repo, "develop", "origin/develop");

    git_test::switch_branch(repo, "develop");

    const fs::path config = temp.path() / "config.yml";
    write_file(
        config,
        "trees:\n"
        "  - root: " +
            root.string() +
            "\n"
            "    repos:\n"
            "      - name: repo1\n"
            "        type: git\n"
            "        remotes:\n"
            "          - name: origin\n"
            "            url: " +
            origin_remote.string() +
            "\n"
            "        branches:\n"
            "          - name: master\n"
            "            remote: origin\n"
            "            is_current: true\n"
            "          - name: develop\n"
            "            remote: origin\n"
            "            is_current: false\n");

    int result = run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = false,
            .jobs = 0,
        });

    EXPECT_EQ(0, result);

    auto branches = GitManager::get_branches(repo.string());
    auto current_it = std::ranges::find_if(branches, [](const Branch &b) {
        return b.is_current;
    });
    ASSERT_NE(current_it, branches.end());
    EXPECT_EQ("master", current_it->name);
}

TEST(SyncTests, EmitsTextOutputWhenNotInTerminal) {
    if (detect_output_mode() != OutputMode::TEXT) {
        GTEST_SKIP() << "Only valid for non-terminal execution mode";
    }

    testing::internal::CaptureStdout();
    sync("nested_repos.yml");
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(std::string::npos, out.find("mrm report")) << out;
    EXPECT_NE(std::string::npos, out.find("[SUCCEEDED] nested/parent")) << out;
    EXPECT_NE(std::string::npos, out.find("SUCCEEDED")) << out;
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
    git_test::init_repo(remote, "main", true);
    git_test::init_repo(repo, "main");
    git_test::set_user(repo, "test@example.com", "test");

    write_file(repo / "README.md", "hello\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", remote);
    git_test::push_branch(repo, "origin", "main", true);
    git_test::set_remote_url(repo, "origin", invalid_remote);

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
    run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .jobs = 0,
        });
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
    git_test::init_repo(remote, "main", true);
    git_test::init_repo(upstream, "main", true);
    git_test::init_repo(repo, "main");
    git_test::set_user(repo, "test@example.com", "test");
    write_file(repo / "README.md", "hello\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", remote);
    git_test::push_branch(repo, "origin", "main", true);
    git_test::add_remote(repo, "upstream", upstream);

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .jobs = 0,
        });
    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_TRUE(has_remote(repos[0], "upstream"));

    run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = true,
            .prune_branches = false,
            .jobs = 0,
        });
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
    git_test::init_repo(remote, "main", true);
    git_test::init_repo(repo, "main");
    git_test::set_user(repo, "test@example.com", "test");
    write_file(repo / "README.md", "hello\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", remote);
    git_test::push_branch(repo, "origin", "main", true);
    git_test::create_and_checkout_branch(repo, "feature");
    write_file(repo / "feature.txt", "feature\n");
    git_test::stage_all(repo);
    git_test::commit(repo, "feature");
    git_test::push_branch(repo, "origin", "feature", true);
    git_test::checkout_branch(repo, "main");

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = true,
            .jobs = 0,
        });
    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_NE(find_branch(repos[0], "main"), nullptr);
    EXPECT_EQ(find_branch(repos[0], "feature"), nullptr);
}

TEST(SyncTests, PruneReposRemovesUntrackedRepositories) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path remote = temp.path() / "remote.git";
    const fs::path repo1 = root / "repo1";
    const fs::path repo2 = root / "repo2";
    fs::create_directories(repo1);
    fs::create_directories(repo2);

    git_test::init_repo(remote, "main", true);
    git_test::init_repo(repo1, "main");
    git_test::set_user(repo1, "test@example.com", "test");
    write_file(repo1 / "README.md", "hello\n");
    git_test::stage_all(repo1);
    git_test::commit(repo1, "init");
    git_test::add_remote(repo1, "origin", remote);

    git_test::init_repo(repo2, "main");
    git_test::set_user(repo2, "test@example.com", "test");
    write_file(repo2 / "README.md", "untracked\n");
    git_test::stage_all(repo2);
    git_test::commit(repo2, "init");

    const fs::path config = temp.path() / "config.yml";
    write_main_only_config(config, root, remote);

    run_sync(
        SyncOptions{
            .config_file = config.string(),
            .root_patterns = {},
            .prune_remotes = false,
            .prune_branches = false,
            .prune_repos = true,
            .jobs = 0,
        });

    auto repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("repo1", repos[0].name);
    EXPECT_TRUE(fs::exists(repo1));
    EXPECT_FALSE(fs::exists(repo2));
}
