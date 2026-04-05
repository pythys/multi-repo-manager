#include "completion/completion_spec.hpp"
#include <yaml-cpp/yaml.h>

namespace {

std::string format_value_hint(ValueHint hint) {
    switch (hint) {
    case ValueHint::None:
        return "none";
    case ValueHint::File:
        return "file";
    case ValueHint::Dir:
        return "dir";
    case ValueHint::Enum:
        return "enum";
    case ValueHint::Command:
        return "command";
    }
    return "unknown";
}

YAML::Node value_spec_node(const ValueSpec &spec) {
    YAML::Node node;
    node["hint"] = format_value_hint(spec.hint);
    if (!spec.choices.empty()) {
        YAML::Node choices(YAML::NodeType::Sequence);
        for (const auto &choice : spec.choices) {
            choices.push_back(choice);
        }
        node["choices"] = choices;
    }
    if (!spec.command.empty()) {
        node["command"] = spec.command;
    }
    return node;
}

YAML::Node option_node(const OptionSpec &option) {
    YAML::Node node;
    YAML::Node flags(YAML::NodeType::Sequence);
    for (const auto &flag : option.flags) {
        flags.push_back(flag);
    }
    node["flags"] = flags;
    if (!option.description.empty()) {
        node["description"] = option.description;
    }
    node["takes_value"] = option.takes_value;
    node["repeatable"] = option.repeatable;
    if (option.takes_value) {
        node["value"] = value_spec_node(option.value);
    }
    return node;
}

YAML::Node arg_node(const ArgSpec &arg) {
    YAML::Node node;
    node["name"] = arg.name;
    if (!arg.description.empty()) {
        node["description"] = arg.description;
    }
    node["optional"] = arg.optional;
    node["repeatable"] = arg.repeatable;
    node["value"] = value_spec_node(arg.values);
    return node;
}

YAML::Node command_node(const CommandSpec &command) {
    YAML::Node node;
    node["name"] = command.name;
    if (!command.description.empty()) {
        node["description"] = command.description;
    }

    YAML::Node options(YAML::NodeType::Sequence);
    for (const auto &option : command.options) {
        options.push_back(option_node(option));
    }
    node["options"] = options;

    YAML::Node positionals(YAML::NodeType::Sequence);
    for (const auto &arg : command.positionals) {
        positionals.push_back(arg_node(arg));
    }
    node["positionals"] = positionals;

    YAML::Node subcommands(YAML::NodeType::Sequence);
    for (const auto &subcommand : command.subcommands) {
        subcommands.push_back(command_node(subcommand));
    }
    node["subcommands"] = subcommands;
    return node;
}

} // namespace

std::string render_spec(const CompletionSpec &spec) {
    YAML::Node root;
    root["root"] = command_node(spec.root);
    YAML::Emitter out;
    out << root;
    return {out.c_str()};
}
