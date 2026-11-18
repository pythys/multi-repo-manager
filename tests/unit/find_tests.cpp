#include "command/find.hpp"
#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include "test_utils.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
const GitGuard git_guard;

namespace {
using test_utils::TempDir;
} // namespace

TEST(FindTests, MultiplePathsBecomeMultipleTrees) {
    TempDir temp;
    const fs::path first = temp.path() / "first";
    const fs::path second = temp.path() / "second";
    const fs::path output = temp.path() / "repos.yml";
    fs::create_directories(first);
    fs::create_directories(second);

    const std::vector<std::string> paths = {first.string(), second.string()};
    ASSERT_EQ(0, run_find(paths, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(2, trees.size());
    EXPECT_EQ(first.lexically_normal().string(), trees[0].root);
    EXPECT_EQ(second.lexically_normal().string(), trees[1].root);
}

TEST(NormalizePathTests, StripsTrailingSlashFromCompletedPath) {
    EXPECT_EQ("fork/moqui-framework", normalize_path("fork/moqui-framework/"));
    EXPECT_EQ("fork", normalize_path("./fork/"));
    EXPECT_EQ("a/b", normalize_path("a/b/./"));
}

TEST(NormalizePathTests, PreservesRootAndDotPaths) {
    EXPECT_EQ("/", normalize_path("/"));
    EXPECT_EQ(".", normalize_path("."));
    EXPECT_EQ(".", normalize_path("./"));
}

TEST(FindTests, TrailingSlashRootMatchesFilter) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path output = temp.path() / "repos.yml";
    fs::create_directories(root);
    GitManager::init(root.string(), "master");

    ASSERT_EQ(0, run_find({root.string() + "/"}, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    EXPECT_NE('/', trees[0].root.back());
    EXPECT_EQ(root.lexically_normal().string(), trees[0].root);
}

TEST(FindTests, EmptyPathsDefaultToCurrentDirectory) {
    test_utils::ScopedTempCwd scratch;
    const fs::path output = scratch.path() / "repos.yml";

    ASSERT_EQ(0, run_find({}, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ(".", trees[0].root);
}

TEST(FindTests, RootThatIsItselfARepoIsIncluded) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    fs::create_directories(root);
    GitManager::init(root.string(), "master");

    const std::vector<Repo> repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ(".", repos[0].name);
    EXPECT_EQ(fs::canonical(root / repos[0].name), fs::canonical(root));
}

TEST(FindTests, MindepthExcludesRootButKeepsNested) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path nested = root / "child";
    fs::create_directories(nested);
    GitManager::init(root.string(), "master");
    GitManager::init(nested.string(), "master");

    const std::vector<Repo> included = find_repos(root.string());
    ASSERT_EQ(2, included.size());
    EXPECT_EQ(".", included[0].name);
    EXPECT_EQ("child", included[1].name);

    const std::vector<Repo> excluded = find_repos(root.string(), 1);
    ASSERT_EQ(1, excluded.size());
    EXPECT_EQ("child", excluded[0].name);
}

TEST(FindTests, MaxdepthZeroKeepsOnlyRoot) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path nested = root / "child";
    fs::create_directories(nested);
    GitManager::init(root.string(), "master");
    GitManager::init(nested.string(), "master");

    const std::vector<Repo> repos = find_repos(root.string(), 0, 0);
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ(".", repos[0].name);
}

TEST(FindTests, MaxdepthLimitsNestedRepos) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path child = root / "child";
    const fs::path grandchild = child / "grandchild";
    fs::create_directories(grandchild);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");
    GitManager::init(grandchild.string(), "master");

    const std::vector<Repo> shallow = find_repos(root.string(), 0, 1);
    ASSERT_EQ(2, shallow.size());
    EXPECT_EQ(".", shallow[0].name);
    EXPECT_EQ("child", shallow[1].name);

    const std::vector<Repo> deep = find_repos(root.string(), 0, 2);
    ASSERT_EQ(3, deep.size());
    EXPECT_EQ(".", deep[0].name);
    EXPECT_EQ("child", deep[1].name);
    EXPECT_EQ("child/grandchild", deep[2].name);
}

TEST(FindTests, DepthRangeKeepsOnlyRequestedLevel) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path child = root / "child";
    const fs::path grandchild = child / "grandchild";
    fs::create_directories(grandchild);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");
    GitManager::init(grandchild.string(), "master");

    const std::vector<Repo> repos = find_repos(root.string(), 1, 1);
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("child", repos[0].name);
}

TEST(FindTests, DepthIgnoresIntermediateDirectories) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path child = root / "layers" / "deep" / "child";
    fs::create_directories(child);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");

    const std::vector<Repo> shallow = find_repos(root.string(), 0, 1);
    ASSERT_EQ(2, shallow.size());
    EXPECT_EQ(".", shallow[0].name);
    EXPECT_EQ("layers/deep/child", shallow[1].name);

    const std::vector<Repo> child_only = find_repos(root.string(), 1, 1);
    ASSERT_EQ(1, child_only.size());
    EXPECT_EQ("layers/deep/child", child_only[0].name);

    EXPECT_TRUE(find_repos(root.string(), 2).empty());
}

TEST(FindTests, DepthFollowsRepositoryAncestry) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path child = root / "vendor" / "plugins" / "app";
    const fs::path grandchild = child / "libs" / "dep";
    fs::create_directories(grandchild);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");
    GitManager::init(grandchild.string(), "master");

    const std::vector<Repo> shallow = find_repos(root.string(), 0, 1);
    ASSERT_EQ(2, shallow.size());
    EXPECT_EQ(".", shallow[0].name);
    EXPECT_EQ("vendor/plugins/app", shallow[1].name);

    const std::vector<Repo> deep = find_repos(root.string(), 0, 2);
    ASSERT_EQ(3, deep.size());
    EXPECT_EQ("vendor/plugins/app", deep[1].name);
    EXPECT_EQ("vendor/plugins/app/libs/dep", deep[2].name);

    const std::vector<Repo> grandchild_only = find_repos(root.string(), 2, 2);
    ASSERT_EQ(1, grandchild_only.size());
    EXPECT_EQ("vendor/plugins/app/libs/dep", grandchild_only[0].name);
}

TEST(FindTests, TopLevelReposStartAtOneWhenRootIsNotRepo) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path child = root / "layers" / "deep" / "child";
    fs::create_directories(child);
    GitManager::init(child.string(), "master");

    EXPECT_TRUE(find_repos(root.string(), 0, 0).empty());

    const std::vector<Repo> repos = find_repos(root.string(), 1, 1);
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("layers/deep/child", repos[0].name);

    EXPECT_TRUE(find_repos(root.string(), 2).empty());
}

TEST(FindTests, TrailingSlashRootComputesRepositoryDepth) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path child = root / "layers" / "child";
    fs::create_directories(child);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");

    const std::vector<Repo> repos = find_repos(root.string() + "/", 0, 1);
    ASSERT_EQ(2, repos.size());
    EXPECT_EQ(".", repos[0].name);
    EXPECT_EQ("layers/child", repos[1].name);
}

TEST(FindTests, TrailingSlashRootWithoutRootRepoComputesDepth) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path child = root / "layers" / "child";
    fs::create_directories(child);
    GitManager::init(child.string(), "master");

    const std::vector<Repo> repos = find_repos(root.string() + "/", 0, 1);
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("layers/child", repos[0].name);
}

TEST(FindTests, RunFindHonorsMaxdepth) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path child = root / "child";
    const fs::path grandchild = child / "grandchild";
    const fs::path output = temp.path() / "repos.yml";
    fs::create_directories(grandchild);
    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");
    GitManager::init(grandchild.string(), "master");

    ASSERT_EQ(0, run_find({root.string()}, output.string(), 0, 1));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    ASSERT_EQ(2, trees[0].repos.size());
    EXPECT_EQ(".", trees[0].repos[0].name);
    EXPECT_EQ("child", trees[0].repos[1].name);
}

TEST(FindTests, SkipsSymbolicLinkToRepository) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path repo = root / "repo";
    fs::create_directories(repo / ".openclaw");
    GitManager::init(repo.string(), "master");
    fs::create_directory_symlink("..", repo / ".openclaw" / "repo");

    const std::vector<Repo> repos = find_repos(root.string());
    ASSERT_EQ(1, repos.size());
    EXPECT_EQ("repo", repos[0].name);
}

TEST(FindTests, RootRepoNameRoundTrips) {
    TempDir temp;
    const fs::path root = temp.path() / "repo";
    const fs::path output = temp.path() / "repos.yml";
    fs::create_directories(root);
    GitManager::init(root.string(), "master");

    ASSERT_EQ(0, run_find({root.string()}, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    ASSERT_EQ(1, trees[0].repos.size());
    EXPECT_EQ(".", trees[0].repos[0].name);
}

TEST(FindTests, PermissionDeniedEntryIsIgnored) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path blocked = root / "blocked";
    const fs::path output = temp.path() / "repos.yml";
    fs::create_directories(blocked / "nested");

    std::error_code ec;
    fs::permissions(blocked, fs::perms::none, fs::perm_options::replace, ec);
    ASSERT_FALSE(ec);

    ASSERT_EQ(0, run_find({root.string()}, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ(root.lexically_normal().string(), trees[0].root);

    fs::permissions(
        blocked,
        fs::perms::owner_all,
        fs::perm_options::replace,
        ec);
}
