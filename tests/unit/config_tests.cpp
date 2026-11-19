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

std::string config_scenario(const std::string &filename) {
    return std::string(TEST_RESOURCES_DIR) + "/scenarios/config/" + filename;
}

std::vector<Tree> parse_config(const std::string &filename) {
    return get_config(config_scenario(filename));
}

std::vector<Tree> parse_dependencies(const std::string &filename) {
    return get_dependencies(config_scenario(filename));
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

TEST(ConfigTests, RootRepoBecomesParentOfNested) {
    TempDir temp;
    const std::string config = (temp.path() / "root_repo.yml").string();
    const std::vector<Tree> input = {
        make_test_tree(".", {make_test_repo("."), make_test_repo("child")}),
    };
    write_config(input, config);

    const std::vector<Tree> trees = get_dependencies(config);
    ASSERT_EQ(1, trees.size());
    ASSERT_EQ(1, trees[0].repos.size());
    EXPECT_EQ(".", trees[0].repos[0].name);
    ASSERT_EQ(1, trees[0].repos[0].children.size());
    EXPECT_EQ("child", trees[0].repos[0].children[0].name);
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
            {
                make_test_repo("frontend"),
                make_test_repo("backend"),
                make_test_repo("database"),
            }),
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
            {
                make_test_repo("api-gateway"),
                make_test_repo("api-service"),
                make_test_repo("frontend-app"),
            }),
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
            {
                make_test_repo("frontend"),
                make_test_repo("backend"),
                make_test_repo("mobile"),
            }),
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
            {
                make_test_repo("app1"),
                make_test_repo("app2"),
                make_test_repo("app12"),
            }),
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

TEST(ConfigTests, LoadTreesFromConfigHonorsDepthRange) {
    const std::string config = config_scenario("nested_repositories.yml");

    const auto shallow = load_trees(config, {}, {}, {}, 0, 1);
    ASSERT_EQ(1, shallow.size());
    ASSERT_EQ(1, shallow[0].repos.size());
    EXPECT_EQ("parent", shallow[0].repos[0].name);

    const auto middle = load_trees(config, {}, {}, {}, 2, 2);
    ASSERT_EQ(1, middle.size());
    ASSERT_EQ(2, middle[0].repos.size());
    EXPECT_EQ("parent/child1", middle[0].repos[0].name);
    EXPECT_EQ("parent/child2", middle[0].repos[1].name);

    const auto deep = load_trees(config, {}, {}, {}, 3, 3);
    ASSERT_EQ(1, deep.size());
    ASSERT_EQ(1, deep[0].repos.size());
    EXPECT_EQ("parent/child1/not_a_repo/grandchild1", deep[0].repos[0].name);

    EXPECT_TRUE(load_trees(config, {}, {}, {}, 0, 0).empty());
}

TEST(ConfigTests, LoadTreesFromConfigHonorsRootRepoDepth) {
    const std::string config = config_scenario("depth_ranges.yml");

    const auto root_only = load_trees(config, {}, {}, {}, 0, 0);
    ASSERT_EQ(1, root_only.size());
    ASSERT_EQ(1, root_only[0].repos.size());
    EXPECT_EQ(".", root_only[0].repos[0].name);

    const auto top = load_trees(config, {}, {}, {}, 0, 1);
    ASSERT_EQ(1, top.size());
    ASSERT_EQ(3, top[0].repos.size());
    EXPECT_EQ(".", top[0].repos[0].name);
    EXPECT_EQ("app", top[0].repos[1].name);
    EXPECT_EQ("vendor/widgets", top[0].repos[2].name);

    const auto second = load_trees(config, {}, {}, {}, 2, 2);
    ASSERT_EQ(1, second.size());
    ASSERT_EQ(1, second[0].repos.size());
    EXPECT_EQ("app/plugins/auth", second[0].repos[0].name);

    const auto third = load_trees(config, {}, {}, {}, 3, 3);
    ASSERT_EQ(1, third.size());
    ASSERT_EQ(1, third[0].repos.size());
    EXPECT_EQ("app/plugins/auth/vendor/deep", third[0].repos[0].name);
}

TEST(ConfigTests, ConfigDepthAppliesBeforeNameFiltering) {
    const std::string config = config_scenario("nested_repositories.yml");

    EXPECT_TRUE(load_trees(config, {}, {}, {"*grandchild*"}, 0, 2).empty());

    const auto deep = load_trees(config, {}, {}, {"*grandchild*"}, 3, 3);
    ASSERT_EQ(1, deep.size());
    ASSERT_EQ(1, deep[0].repos.size());
    EXPECT_EQ("parent/child1/not_a_repo/grandchild1", deep[0].repos[0].name);
}

TEST(ConfigTests, RejectsDuplicateRepositories) {
    TempDir temp;
    const std::string config = (temp.path() / "repos.yml").string();
    write_config(
        {
            make_test_tree(
                "projects",
                {make_test_repo("app"), make_test_repo("app")}),
        },
        config);

    EXPECT_THROW(get_config(config), std::runtime_error);
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

TEST(ConfigTests, LoadTreesFromFindPathsHonorsMaxDepth) {
    test_utils::TempDir temp;

    const auto root = temp.path() / "root";
    const auto child = root / "a" / "child";
    const auto grandchild = child / "b" / "grandchild";
    fs::create_directories(grandchild);

    GitManager::init(root.string(), "master");
    GitManager::init(child.string(), "master");
    GitManager::init(grandchild.string(), "master");

    const auto trees = load_trees("", {root.string()}, {}, {}, 0, 1);
    ASSERT_EQ(trees.size(), 1);
    ASSERT_EQ(trees[0].repos.size(), 2);
    EXPECT_EQ(trees[0].repos[0].name, ".");
    EXPECT_EQ(trees[0].repos[1].name, "a/child");
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

namespace {
void commit_scenario(
    const fs::path &repo_path,
    const std::string &name,
    const std::string &fixture) {
    fs::copy_file(
        config_scenario(fixture),
        repo_path / name,
        fs::copy_options::overwrite_existing);
    GitManager::commit(repo_path.string(), "commit " + name);
}
} // namespace

TEST(ConfigTests, GetConfigMissingLocalFileThrows) {
    EXPECT_THROW(
        get_config("definitely-missing-config.yml"),
        std::runtime_error);
}

TEST(ConfigTests, GetConfigMissingTreesNodeIsEmpty) {
    EXPECT_TRUE(get_config(config_scenario("missing_trees.yml")).empty());
}

TEST(ConfigTests, GetConfigMalformedYamlIsGraceful) {
    EXPECT_TRUE(get_config(config_scenario("malformed_trees.yml")).empty());
}

TEST(ConfigTests, GetConfigRejectsPathTraversal) {
    EXPECT_THROW(get_config("repo//../secret.yml"), std::runtime_error);
}

TEST(ConfigTests, GetConfigFetchesFromLocalRepoAtRef) {
    TempDir temp;
    const fs::path repo = temp.path() / "infra";
    GitManager::init(repo.string(), "master");
    commit_scenario(repo, "repos.yml", "remote_single_repo.yml");
    GitManager::create_branch(repo.string(), "stable");

    const std::string source = repo.string() + "//repos.yml?ref=stable";
    const std::vector<Tree> trees = get_config(source);
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ("demo", trees[0].root);
}

TEST(ConfigTests, GetConfigFetchesFromLocalRepo) {
    TempDir temp;
    const fs::path repo = temp.path() / "infra";
    GitManager::init(repo.string(), "master");
    commit_scenario(repo, "repos.yml", "remote_single_repo.yml");

    const std::string source = repo.string() + "//repos.yml";
    const std::vector<Tree> trees = get_config(source);
    ASSERT_EQ(1, trees.size());
    EXPECT_EQ("demo", trees[0].root);
    ASSERT_EQ(1, trees[0].repos.size());
    EXPECT_EQ("app", trees[0].repos[0].name);
}
