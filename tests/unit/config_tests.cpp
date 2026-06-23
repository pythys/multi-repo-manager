#include "command/find.hpp"
#include "persistence/config.hpp"
#include "test_utils.hpp"
#include "vcs/git_guard.hpp"
#include "vcs/git_manager.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::vector<Tree> parse_config(const std::string &filename) {
    return get_config(
        std::string(TEST_RESOURCES_DIR) + "/scenarios/config/" + filename);
}

std::vector<Tree> parse_dependencies(const std::string &filename) {
    return get_dependencies(
        std::string(TEST_RESOURCES_DIR) + "/scenarios/config/" + filename);
}

namespace {
const GitGuard git_guard;
using test_utils::TempDir;

Repo make_test_repo(const std::string &name) {
    return Repo{
        .name = name,

        .remotes = {},
        .branches = {},
        .children = {},
        .messages = {},
    };
}

Tree make_test_tree(const std::string &root, std::vector<Repo> repos) {
    return Tree{.root = root, .repos = std::move(repos)};
}
} // namespace

TEST(ConfigTests, WithoutNesting) {
    std::vector<Tree> trees = parse_config("nested_repositories.yml");
    EXPECT_EQ(trees[0].repos.size(), 4);
}

TEST(ConfigTests, NestingFirstLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repositories.yml");
    EXPECT_EQ(trees[0].repos.size(), 1);
}

TEST(ConfigTests, NestingSecondLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repositories.yml");
    EXPECT_EQ(trees[0].repos[0].children.size(), 2);
}

TEST(ConfigTests, NestingThirdLevel) {
    std::vector<Tree> trees = parse_dependencies("nested_repositories.yml");
    EXPECT_EQ(trees[0].repos[0].children[0].children.size(), 1);
    EXPECT_EQ(trees[0].repos[0].children[1].children.size(), 0);
}

TEST(ConfigTests, MultipleTrees) {
    std::vector<Tree> trees = parse_dependencies("multiple_trees.yml");
    EXPECT_EQ(trees.size(), 2);
}

TEST(ConfigTests, FilterTreesByExactRoot) {
    const std::vector<Tree> trees = {
        Tree{.root = "r/client", .repos = {}},
        Tree{.root = "r/fork", .repos = {}},
        Tree{.root = "r/personal", .repos = {}},
    };
    const std::vector<Tree> filtered = filter_trees_by_root(trees, {"r/fork"});
    ASSERT_EQ(1, filtered.size());
    EXPECT_EQ("r/fork", filtered[0].root);
}

TEST(ConfigTests, FilterTreesByPattern) {
    const std::vector<Tree> trees = {
        Tree{.root = "r/client", .repos = {}},
        Tree{.root = "r/fork", .repos = {}},
        Tree{.root = "r/personal", .repos = {}},
    };
    const std::vector<Tree> filtered =
        filter_trees_by_root(trees, {"r/*", "*personal"});
    ASSERT_EQ(3, filtered.size());
}

TEST(ConfigTests, FilterTreesByNameExactMatch) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("frontend"),
             make_test_repo("backend"),
             make_test_repo("database")}),
    };
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {"backend"});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(1, filtered[0].repos.size());
    EXPECT_EQ("backend", filtered[0].repos[0].name);
}

TEST(ConfigTests, FilterTreesByNameWildcardPattern) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("api-gateway"),
             make_test_repo("api-service"),
             make_test_repo("frontend-app")}),
    };
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {"*api*"});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(2, filtered[0].repos.size());
    EXPECT_EQ("api-gateway", filtered[0].repos[0].name);
    EXPECT_EQ("api-service", filtered[0].repos[1].name);
}

TEST(ConfigTests, FilterTreesByNameMultiplePatterns) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("frontend"),
             make_test_repo("backend"),
             make_test_repo("mobile")}),
    };
    const std::vector<Tree> filtered =
        filter_trees_by_name(trees, {"frontend", "mobile"});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(2, filtered[0].repos.size());
    EXPECT_EQ("frontend", filtered[0].repos[0].name);
    EXPECT_EQ("mobile", filtered[0].repos[1].name);
}

TEST(ConfigTests, FilterTreesByNamePrunesEmptyTrees) {
    const std::vector<Tree> trees = {
        make_test_tree("projects1", {make_test_repo("frontend")}),
        make_test_tree("projects2", {make_test_repo("backend")}),
    };
    const std::vector<Tree> filtered =
        filter_trees_by_name(trees, {"frontend"});
    ASSERT_EQ(1, filtered.size());
    EXPECT_EQ("projects1", filtered[0].root);
}

TEST(ConfigTests, FilterTreesByNameNoMatch) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("frontend"), make_test_repo("backend")}),
    };
    const std::vector<Tree> filtered =
        filter_trees_by_name(trees, {"nonexistent"});
    EXPECT_EQ(0, filtered.size());
}

TEST(ConfigTests, FilterTreesByNameEmptyPatterns) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("frontend"), make_test_repo("backend")}),
    };
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(2, filtered[0].repos.size());
}

TEST(ConfigTests, FilterTreesByNameQuestionMarkWildcard) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("app1"),
             make_test_repo("app2"),
             make_test_repo("app12")}),
    };
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {"app?"});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(2, filtered[0].repos.size());
    EXPECT_EQ("app1", filtered[0].repos[0].name);
    EXPECT_EQ("app2", filtered[0].repos[1].name);
}

TEST(ConfigTests, FilterTreesByNameCaseSensitive) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "projects",
            {make_test_repo("Frontend"), make_test_repo("frontend")}),
    };
    const std::vector<Tree> filtered =
        filter_trees_by_name(trees, {"frontend"});
    ASSERT_EQ(1, filtered.size());
    ASSERT_EQ(1, filtered[0].repos.size());
    EXPECT_EQ("frontend", filtered[0].repos[0].name);
}

TEST(ConfigTests, CombinedFilterRootThenName) {
    const std::vector<Tree> trees = {
        make_test_tree(
            "work/client",
            {make_test_repo("api"), make_test_repo("frontend")}),
        make_test_tree(
            "work/internal",
            {make_test_repo("api"), make_test_repo("backend")}),
        make_test_tree("personal/hobby", {make_test_repo("api")}),
    };
    auto filtered = filter_trees_by_root(trees, {"work/*"});
    filtered = filter_trees_by_name(filtered, {"api"});
    ASSERT_EQ(2, filtered.size());
    EXPECT_EQ("work/client", filtered[0].root);
    EXPECT_EQ("work/internal", filtered[1].root);
    ASSERT_EQ(1, filtered[0].repos.size());
    ASSERT_EQ(1, filtered[1].repos.size());
    EXPECT_EQ("api", filtered[0].repos[0].name);
    EXPECT_EQ("api", filtered[1].repos[0].name);
}

TEST(ConfigTests, CombinedFilterPrunesAllTrees) {
    const std::vector<Tree> trees = {
        make_test_tree("work/client", {make_test_repo("frontend")}),
        make_test_tree("work/internal", {make_test_repo("backend")}),
    };
    auto filtered = filter_trees_by_root(trees, {"work/*"});
    filtered = filter_trees_by_name(filtered, {"nonexistent"});
    EXPECT_EQ(0, filtered.size());
}

TEST(ConfigTests, CombinedFilterNameBeforeRootPruning) {
    const std::vector<Tree> trees = {
        make_test_tree("work/client", {make_test_repo("api")}),
        make_test_tree("personal/hobby", {make_test_repo("backend")}),
    };
    auto filtered = filter_trees_by_root(trees, {"work/*"});
    filtered = filter_trees_by_name(filtered, {"backend"});
    EXPECT_EQ(0, filtered.size());
}

TEST(ConfigTests, FilterTreesByNameEmptyTreesList) {
    const std::vector<Tree> trees = {};
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {"pattern"});
    EXPECT_EQ(0, filtered.size());
}

TEST(ConfigTests, FilterTreesByNameTreeWithNoRepos) {
    const std::vector<Tree> trees = {
        make_test_tree("empty-tree", {}),
        make_test_tree("projects", {make_test_repo("api")}),
    };
    const std::vector<Tree> filtered = filter_trees_by_name(trees, {"api"});
    ASSERT_EQ(1, filtered.size());
    EXPECT_EQ("projects", filtered[0].root);
    ASSERT_EQ(1, filtered[0].repos.size());
}

TEST(ConfigTests, LoadTreesFromConfigFile) {
    const auto trees = load_trees(
        std::string(TEST_RESOURCES_DIR) +
            "/scenarios/config/nested_repositories.yml",
        {},
        {},
        {});
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ("nested", trees[0].root);
    EXPECT_EQ(4, trees[0].repos.size());
}

TEST(ConfigTests, LoadTreesWithRootFilter) {
    const auto trees = load_trees(
        std::string(TEST_RESOURCES_DIR) +
            "/scenarios/config/multiple_trees.yml",
        {},
        {"first*"},
        {});
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ("first_root", trees[0].root);
}

TEST(ConfigTests, LoadTreesWithNameFilter) {
    const auto trees = load_trees(
        std::string(TEST_RESOURCES_DIR) +
            "/scenarios/config/nested_repositories.yml",
        {},
        {},
        {"parent"});
    ASSERT_EQ(1, trees.size());
    ASSERT_EQ(1, trees[0].repos.size());
    EXPECT_EQ("parent", trees[0].repos[0].name);
}

TEST(ConfigTests, LoadTreesWithBothFilters) {
    const auto trees = load_trees(
        std::string(TEST_RESOURCES_DIR) +
            "/scenarios/config/multiple_trees.yml",
        {},
        {"second*"},
        {"*d"});
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ("second_root", trees[0].root);
    ASSERT_EQ(1, trees[0].repos.size());
    EXPECT_EQ("fd", trees[0].repos[0].name);
}

TEST(ConfigTests, LoadTreesEmptyAfterFiltering) {
    const auto trees = load_trees(
        std::string(TEST_RESOURCES_DIR) +
            "/scenarios/config/nested_repositories.yml",
        {},
        {},
        {"nonexistent-repo-name"});
    EXPECT_EQ(0, trees.size());
}

TEST(ConfigTests, LoadTreesFromFindPaths) {
    test_utils::TempDir temp;

    const auto test_dir = temp.path() / "test_basic";
    fs::create_directories(test_dir);

    GitManager::init(test_dir.string(), "main");

    const auto trees = load_trees("", {test_dir.string()}, {}, {});
    EXPECT_GE(trees.size(), 1);
    if (!trees.empty()) {
        EXPECT_EQ(
            fs::canonical(trees[0].root).string(),
            fs::canonical(test_dir).string());
    }
}

TEST(ConfigTests, LoadTreesFromMultipleFindPaths) {
    test_utils::TempDir temp;

    const auto test_dir1 = temp.path() / "test_basic";
    const auto test_dir2 = temp.path() / "test_nested";
    fs::create_directories(test_dir1);
    fs::create_directories(test_dir2);

    GitManager::init(test_dir1.string(), "main");
    GitManager::init(test_dir2.string(), "main");

    const auto trees =
        load_trees("", {test_dir1.string(), test_dir2.string()}, {}, {});
    EXPECT_EQ(trees.size(), 2);
}

TEST(ConfigTests, LoadTreesFromFindWithNameFilter) {
    test_utils::TempDir temp;

    const auto parent_dir = temp.path() / "parent";
    const auto test_dir1 = parent_dir / "test_basic";
    const auto test_dir2 = parent_dir / "test_nested";
    fs::create_directories(test_dir1);
    fs::create_directories(test_dir2);

    GitManager::init(test_dir1.string(), "main");
    GitManager::init(test_dir2.string(), "main");

    const auto trees = load_trees("", {parent_dir.string()}, {}, {"*basic*"});
    EXPECT_EQ(trees.size(), 1);
    if (!trees.empty()) {
        EXPECT_EQ(trees[0].repos.size(), 1);
        EXPECT_EQ(trees[0].repos[0].name, "test_basic");
    }
}

TEST(ConfigTests, LoadTreesFromFindInvalidPath) {
    TempDir temp;
    const fs::path nonexistent = temp.path() / "does-not-exist";
    EXPECT_THROW(
        load_trees("", {nonexistent.string()}, {}, {}),
        std::runtime_error);
}
