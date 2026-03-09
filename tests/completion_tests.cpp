#include "completion.hpp"
#include <gtest/gtest.h>

namespace {

const OptionSpec *
find_option(const CommandSpec &command, const std::string &flag) {
    for (const auto &option : command.options) {
        for (const auto &option_flag : option.flags) {
            if (option_flag == flag) {
                return &option;
            }
        }
    }
    return nullptr;
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

    CompletionSpec spec = extract_spec(app);

    ASSERT_EQ(spec.root.name, "mrm");
    ASSERT_EQ(spec.root.subcommands.size(), 1U);
    const CommandSpec &sync_spec = spec.root.subcommands.front();
    EXPECT_EQ(sync_spec.name, "sync");
    EXPECT_GE(sync_spec.options.size(), 2U);

    const OptionSpec *config_option = find_option(sync_spec, "--config");
    ASSERT_NE(config_option, nullptr);
    EXPECT_TRUE(config_option->takes_value);
    EXPECT_EQ(config_option->value.hint, ValueHint::File);

    const OptionSpec *root_option = find_option(sync_spec, "--root");
    ASSERT_NE(root_option, nullptr);
    EXPECT_TRUE(root_option->takes_value);
    EXPECT_EQ(root_option->value.hint, ValueHint::Dir);

    ASSERT_EQ(sync_spec.positionals.size(), 1U);
    EXPECT_EQ(sync_spec.positionals.front().name, "targets");
}

TEST(CompletionSpecTests, ExtractsNestedSubcommands) {
    CLI::App app("mrm");
    app.name("mrm");

    auto *remote = app.add_subcommand("remote", "Remote operations");
    auto *add = remote->add_subcommand("add", "Add a remote");
    std::string name;
    add->add_option("name", name, "Remote name")->type_name("name");

    CompletionSpec spec = extract_spec(app);

    ASSERT_EQ(spec.root.subcommands.size(), 1U);
    const CommandSpec &remote_spec = spec.root.subcommands.front();
    EXPECT_EQ(remote_spec.name, "remote");
    ASSERT_EQ(remote_spec.subcommands.size(), 1U);
    EXPECT_EQ(remote_spec.subcommands.front().name, "add");
}
