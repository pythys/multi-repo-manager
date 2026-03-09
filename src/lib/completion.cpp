#include "completion.hpp"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

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

void append_indent(std::ostringstream &out, int depth) {
    for (int i = 0; i < depth; ++i) {
        out << "  ";
    }
}

void append_value_spec(
    std::ostringstream &out,
    const ValueSpec &spec,
    int depth) {
    append_indent(out, depth);
    out << "value_hint: " << format_value_hint(spec.hint) << "\n";
    if (!spec.choices.empty()) {
        append_indent(out, depth);
        out << "choices:";
        for (const auto &choice : spec.choices) {
            out << " " << choice;
        }
        out << "\n";
    }
    if (!spec.command.empty()) {
        append_indent(out, depth);
        out << "command: " << spec.command << "\n";
    }
}

void append_option_spec(
    std::ostringstream &out,
    const OptionSpec &option,
    int depth) {
    append_indent(out, depth);
    out << "- flags:";
    for (const auto &flag : option.flags) {
        out << " " << flag;
    }
    out << "\n";
    append_indent(out, depth + 1);
    out << "takes_value: " << (option.takes_value ? "true" : "false") << "\n";
    append_indent(out, depth + 1);
    out << "repeatable: " << (option.repeatable ? "true" : "false") << "\n";
    if (option.takes_value) {
        append_value_spec(out, option.value, depth + 1);
    }
}

void append_arg_spec(std::ostringstream &out, const ArgSpec &arg, int depth) {
    append_indent(out, depth);
    out << "- name: " << arg.name << "\n";
    append_indent(out, depth + 1);
    out << "optional: " << (arg.optional ? "true" : "false") << "\n";
    append_indent(out, depth + 1);
    out << "repeatable: " << (arg.repeatable ? "true" : "false") << "\n";
    append_value_spec(out, arg.values, depth + 1);
}

void append_command_spec(
    std::ostringstream &out,
    const CommandSpec &command,
    int depth) {
    append_indent(out, depth);
    out << "command: " << command.name << "\n";

    append_indent(out, depth + 1);
    out << "options:\n";
    if (command.options.empty()) {
        append_indent(out, depth + 2);
        out << "(none)\n";
    } else {
        for (const auto &option : command.options) {
            append_option_spec(out, option, depth + 2);
        }
    }

    append_indent(out, depth + 1);
    out << "positionals:\n";
    if (command.positionals.empty()) {
        append_indent(out, depth + 2);
        out << "(none)\n";
    } else {
        for (const auto &arg : command.positionals) {
            append_arg_spec(out, arg, depth + 2);
        }
    }

    append_indent(out, depth + 1);
    out << "subcommands:\n";
    if (command.subcommands.empty()) {
        append_indent(out, depth + 2);
        out << "(none)\n";
    } else {
        for (const auto &subcommand : command.subcommands) {
            append_command_spec(out, subcommand, depth + 2);
        }
    }
}

std::string to_lower_copy(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        out.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

std::string trim_string_copy(const std::string &value) {
    const auto is_space = [](unsigned char ch) {
        return std::isspace(ch) != 0;
    };
    std::size_t begin = 0;
    while (begin < value.size() &&
           is_space(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin &&
           is_space(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    if (begin >= end) {
        return "";
    }
    return {
        value.begin() + static_cast<std::ptrdiff_t>(begin),
        value.begin() + static_cast<std::ptrdiff_t>(end)};
}

ValueSpec derive_value_spec_from_type(const std::string &type_name) {
    ValueSpec spec;
    if (type_name.empty()) {
        return spec;
    }
    const std::string lower = to_lower_copy(type_name);
    if (lower.find("file") != std::string::npos) {
        spec.hint = ValueHint::File;
        return spec;
    }
    if (lower.find("directory") != std::string::npos ||
        lower.find("dir") != std::string::npos) {
        spec.hint = ValueHint::Dir;
        return spec;
    }
    if (lower.find("command") != std::string::npos) {
        spec.hint = ValueHint::Command;
        return spec;
    }
    return spec;
}

std::vector<std::string>
parse_enum_description(const std::string &description) {
    if (description.size() < 2 || description.front() != '{' ||
        description.back() != '}') {
        return {};
    }
    std::vector<std::string> values;
    std::string inner = description.substr(1, description.size() - 2);
    std::size_t start = 0;
    while (start <= inner.size()) {
        const std::size_t end = inner.find(',', start);
        const std::size_t token_end =
            end == std::string::npos ? inner.size() : end;
        std::string token =
            trim_string_copy(inner.substr(start, token_end - start));
        if (!token.empty()) {
            values.push_back(token);
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return values;
}

std::vector<std::string> extract_enum_choices(CLI::Option &option) {
    for (int index = 0;; ++index) {
        try {
            const CLI::Validator *validator = option.get_validator(index);
            if (validator == nullptr) {
                continue;
            }
            const std::string description = validator->get_description();
            std::vector<std::string> choices =
                parse_enum_description(description);
            if (!choices.empty()) {
                return choices;
            }
        } catch (const CLI::OptionNotFound &) {
            break;
        }
    }
    return {};
}

OptionSpec extract_option_spec(CLI::Option &option) {
    OptionSpec spec;
    for (const auto &name : option.get_snames()) {
        spec.flags.push_back("-" + name);
    }
    for (const auto &name : option.get_lnames()) {
        spec.flags.push_back("--" + name);
    }
    for (const auto &name : option.get_fnames()) {
        if (name.size() == 1) {
            spec.flags.push_back("-" + name);
        } else {
            spec.flags.push_back("--" + name);
        }
    }
    const bool is_flag =
        option.get_expected_min() == 0 && option.get_expected_max() == 0;
    spec.takes_value = !is_flag;
    if (spec.takes_value) {
        spec.value = derive_value_spec_from_type(option.get_type_name());
        std::vector<std::string> choices = extract_enum_choices(option);
        if (!choices.empty()) {
            spec.value.hint = ValueHint::Enum;
            spec.value.choices = std::move(choices);
        }
    }
    return spec;
}

ArgSpec extract_positional_spec(CLI::Option &option) {
    ArgSpec spec;
    spec.name = option.get_name();
    spec.optional = !option.get_required();
    spec.values = derive_value_spec_from_type(option.get_type_name());
    std::vector<std::string> choices = extract_enum_choices(option);
    if (!choices.empty()) {
        spec.values.hint = ValueHint::Enum;
        spec.values.choices = std::move(choices);
    }
    return spec;
}

CommandSpec extract_command_spec(CLI::App &app) {
    CommandSpec spec;
    spec.name = app.get_name();
    for (CLI::Option *option : app.get_options()) {
        if (option->get_positional()) {
            spec.positionals.push_back(extract_positional_spec(*option));
        } else {
            spec.options.push_back(extract_option_spec(*option));
        }
    }
    for (CLI::App *subcommand :
         app.get_subcommands([](CLI::App *) { return true; })) {
        spec.subcommands.push_back(extract_command_spec(*subcommand));
    }
    return spec;
}

} // namespace

ShellType parse_shell_type(const std::string &shell) {
    static const std::unordered_map<std::string, ShellType> shell_map = {
        {"bash", ShellType::Bash},
        {"zsh", ShellType::Zsh},
        {"powershell", ShellType::PowerShell}};
    return shell_map.at(shell);
}

CompletionSpec extract_spec(CLI::App &app) {
    CompletionSpec spec;
    spec.root = extract_command_spec(app);
    return spec;
}

std::string print_spec(CLI::App &app) {
    const CompletionSpec spec = extract_spec(app);
    std::ostringstream out;
    out << "completion spec\n";
    append_command_spec(out, spec.root, 1);
    return out.str();
}

std::string generate_script(CLI::App &app, ShellType shell) {
    (void)shell;
    return print_spec(app);
}
