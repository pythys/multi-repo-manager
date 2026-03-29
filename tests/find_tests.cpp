#include "config.hpp"
#include "find.hpp"
#include "git_guard.hpp"
#include "test_utils.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
const GitGuard git_guard;

namespace {
using test_utils::CurrentPathGuard;
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

TEST(FindTests, EmptyPathsDefaultToCurrentDirectory) {
    TempDir temp;
    CurrentPathGuard cwd_guard;
    const fs::path output = temp.path() / "repos.yml";
    fs::current_path(temp.path());

    ASSERT_EQ(0, run_find({}, output.string()));

    const std::vector<Tree> trees = get_config(output.string());
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ(".", trees[0].root);
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
