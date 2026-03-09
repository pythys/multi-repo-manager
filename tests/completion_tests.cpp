#include "completion.hpp"
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
