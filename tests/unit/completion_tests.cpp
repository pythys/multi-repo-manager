#include "cli.hpp"
#include "command/completion.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

YAML::Node find_subcommand(const YAML::Node &command, const std::string &name) {
    const YAML::Node subcommands = command["subcommands"];
    if (!subcommands || !subcommands.IsSequence()) {
        return {};
    }
    for (const YAML::Node subcommand : subcommands) {
        const YAML::Node node_name = subcommand["name"];
        if (node_name && node_name.as<std::string>() == name) {
            return subcommand;
        }
    }
    return {};
}

YAML::Node
find_option_with_flag(const YAML::Node &command, const std::string &flag) {
    const YAML::Node options = command["options"];
    if (!options || !options.IsSequence()) {
        return {};
    }
    for (const YAML::Node option : options) {
        const YAML::Node flags = option["flags"];
        if (!flags || !flags.IsSequence()) {
            continue;
        }
        for (const YAML::Node entry : flags) {
            if (entry.as<std::string>() == flag) {
                return option;
            }
        }
    }
    return {};
}

int option_index(const YAML::Node &command, const std::string &flag) {
    const YAML::Node options = command["options"];
    if (!options || !options.IsSequence()) {
        return -1;
    }
    int index = 0;
    for (const YAML::Node option : options) {
        const YAML::Node flags = option["flags"];
        if (flags && flags.IsSequence()) {
            for (const YAML::Node entry : flags) {
                if (entry.as<std::string>() == flag) {
                    return index;
                }
            }
        }
        ++index;
    }
    return -1;
}

YAML::Node find_positional(const YAML::Node &command, const std::string &name) {
    const YAML::Node positionals = command["positionals"];
    if (!positionals || !positionals.IsSequence()) {
        return {};
    }
    for (const YAML::Node positional : positionals) {
        const YAML::Node node_name = positional["name"];
        if (node_name && node_name.as<std::string>() == name) {
            return positional;
        }
    }
    return {};
}

} // namespace

TEST(CompletionSpecTests, ExtractsSubcommandOptionsAndPositionals) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *sync = app.add_subcommand("sync", "Sync repositories");
    std::string config_path;
    sync->add_option("--config,-c", config_path, "Configuration file")
        ->required()
        ->type_name("file");
    std::string root_dir;
    sync->add_option("--root,-r", root_dir, "Root directory")->type_name("dir");
    std::vector<std::string> targets;
    sync->add_option("targets", targets, "Targets")->type_name("pattern");

    const std::string yaml = generate_script(app, "spec");
    const YAML::Node doc = YAML::Load(yaml);
    const YAML::Node root = doc["root"];

    ASSERT_TRUE(root);
    EXPECT_EQ(root["name"].as<std::string>(), "mrm");

    const YAML::Node sync_node = find_subcommand(root, "sync");
    ASSERT_TRUE(sync_node);

    const YAML::Node config_option =
        find_option_with_flag(sync_node, "--config");
    ASSERT_TRUE(config_option);
    EXPECT_TRUE(config_option["takes_value"].as<bool>());
    EXPECT_EQ(config_option["value"]["hint"].as<std::string>(), "file");

    const YAML::Node root_option = find_option_with_flag(sync_node, "--root");
    ASSERT_TRUE(root_option);
    EXPECT_TRUE(root_option["takes_value"].as<bool>());
    EXPECT_EQ(root_option["value"]["hint"].as<std::string>(), "dir");

    const YAML::Node targets_positional = find_positional(sync_node, "targets");
    ASSERT_TRUE(targets_positional);
    EXPECT_TRUE(targets_positional["repeatable"].as<bool>());
}

TEST(CompletionZshTests, RepeatablePositionalUsesStarSlot) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *find = app.add_subcommand("find", "Find repositories");
    std::vector<std::string> paths;
    find->add_option("paths", paths, "paths")->type_name("dir");

    auto *init = app.add_subcommand("init", "Initialize");
    std::string name;
    init->add_option("name", name, "name")->type_name("dir");

    const std::string script = generate_script(app, "zsh");

    EXPECT_NE(script.find("'*:paths:_files -/'"), std::string::npos);
    EXPECT_NE(script.find("'1:name:_files -/'"), std::string::npos);
}

TEST(CompletionZshTests, RepeatableOptionCanRepeatAndDropsExclusion) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *status = app.add_subcommand("status", "Status");
    std::vector<std::string> find_paths;
    status->add_option("--find,-f", find_paths, "find repositories in paths")
        ->type_name("dir");
    std::string config;
    status->add_option("--config,-c", config, "config file")->type_name("file");

    const std::string script = generate_script(app, "zsh");

    EXPECT_NE(
        script.find("'*'{-f,--find}'[find repositories in paths]"),
        std::string::npos);
    EXPECT_EQ(script.find("'(-f --find)'"), std::string::npos);
    EXPECT_NE(script.find("'(-c --config)'{-c,--config}"), std::string::npos);
}

TEST(CompletionSpecTests, ExtractsNestedSubcommands) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *remote = app.add_subcommand("remote", "Remote operations");
    auto *add = remote->add_subcommand("add", "Add a remote");
    std::string name;
    add->add_option("name", name, "Remote name")->type_name("name");

    const std::string yaml = generate_script(app, "spec");
    const YAML::Node doc = YAML::Load(yaml);
    const YAML::Node root = doc["root"];

    ASSERT_TRUE(root);
    const YAML::Node remote_node = find_subcommand(root, "remote");
    ASSERT_TRUE(remote_node);
    const YAML::Node add_node = find_subcommand(remote_node, "add");
    ASSERT_TRUE(add_node);
}

TEST(CompletionSpecTests, ActualCliOrdersDepthAfterRootAndName) {
    std::vector<std::string> args{"mrm", "completion", "spec"};
    std::vector<char *> argv;
    argv.reserve(args.size());
    for (auto &arg : args) {
        argv.push_back(arg.data());
    }

    testing::internal::CaptureStdout();
    const int result = parse_cli(static_cast<int>(argv.size()), argv.data());
    const std::string output = testing::internal::GetCapturedStdout();

    ASSERT_EQ(0, result);
    const YAML::Node root = YAML::Load(output)["root"];
    ASSERT_TRUE(root);

    for (const std::string &command_name :
         {"list", "status", "update", "remotesync", "exec"}) {
        const YAML::Node command = find_subcommand(root, command_name);
        ASSERT_TRUE(command) << command_name;

        const int root_index = option_index(command, "--root");
        const int name_index = option_index(command, "--name");
        const int min_depth_index = option_index(command, "--mindepth");
        const int max_depth_index = option_index(command, "--maxdepth");

        EXPECT_GE(root_index, 0) << command_name;
        EXPECT_GE(name_index, 0) << command_name;
        EXPECT_GE(min_depth_index, 0) << command_name;
        EXPECT_GE(max_depth_index, 0) << command_name;
        EXPECT_LT(root_index, name_index) << command_name;
        EXPECT_LT(name_index, min_depth_index) << command_name;
        EXPECT_LT(min_depth_index, max_depth_index) << command_name;
    }

    const YAML::Node find_command = find_subcommand(root, "find");
    ASSERT_TRUE(find_command);
    EXPECT_LT(
        option_index(find_command, "--save"),
        option_index(find_command, "--mindepth"));
    EXPECT_LT(
        option_index(find_command, "--mindepth"),
        option_index(find_command, "--maxdepth"));
}

TEST(CompletionSpecTests, ActualCliExposesQuietForTrackedCommands) {
    std::vector<std::string> args{"mrm", "completion", "spec"};
    std::vector<char *> argv;
    argv.reserve(args.size());
    for (auto &arg : args) {
        argv.push_back(arg.data());
    }

    testing::internal::CaptureStdout();
    const int result = parse_cli(static_cast<int>(argv.size()), argv.data());
    const std::string output = testing::internal::GetCapturedStdout();

    ASSERT_EQ(0, result);
    const YAML::Node root = YAML::Load(output)["root"];
    ASSERT_TRUE(root);

    for (const std::string &command_name :
         {"sync", "list", "status", "update", "remotesync", "exec"}) {
        const YAML::Node command = find_subcommand(root, command_name);
        ASSERT_TRUE(command) << command_name;

        const YAML::Node quiet = find_option_with_flag(command, "--quiet");
        ASSERT_TRUE(quiet) << command_name;
        EXPECT_FALSE(quiet["takes_value"].as<bool>()) << command_name;
        EXPECT_TRUE(find_option_with_flag(command, "-q")) << command_name;
    }
}

TEST(CompletionSortingTests, PreservesRepositoryFilterOrder) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *list = app.add_subcommand("list", "List repositories");
    std::vector<std::string> root_patterns;
    std::vector<std::string> name_patterns;
    int min_depth = 0;
    int max_depth = 0;
    list->add_option("--root,-r", root_patterns, "root tree pattern");
    list->add_option("--name,-n", name_patterns, "name pattern");
    list->add_option("--mindepth,-l", min_depth, "minimum depth");
    list->add_option("--maxdepth,-L", max_depth, "maximum depth");

    const std::string zsh = generate_script(app, "zsh");
    EXPECT_NE(
        zsh.find("zstyle ':completion:*:*:mrm:*:options' sort false"),
        std::string::npos);
    EXPECT_LT(zsh.find("--root"), zsh.find("--name"));
    EXPECT_LT(zsh.find("--name"), zsh.find("--mindepth"));
    EXPECT_LT(zsh.find("--mindepth"), zsh.find("--maxdepth"));

    const std::string bash = generate_script(app, "bash");
    EXPECT_NE(
        bash.find("complete -o nosort -F _mrm_completion mrm"),
        std::string::npos);
    EXPECT_LT(bash.find("--root"), bash.find("--name"));
    EXPECT_LT(bash.find("--name"), bash.find("--mindepth"));
    EXPECT_LT(bash.find("--mindepth"), bash.find("--maxdepth"));
}
