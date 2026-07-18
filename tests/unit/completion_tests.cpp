#include "command/completion.hpp"
#include <gtest/gtest.h>
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
