#include "completion_spec.hpp"
#include <array>
#include <sstream>

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

std::string yaml_quote(const std::string &value) {
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    constexpr unsigned char control_limit = 0x20;
    constexpr unsigned char hex_mask = 0x0F;
    const std::array<char, 16> hex = {
        '0',
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',
        'A',
        'B',
        'C',
        'D',
        'E',
        'F'};
    for (char ch : value) {
        const auto uch = static_cast<unsigned char>(ch);
        switch (uch) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (uch < control_limit) {
                out += "\\x";
                out.push_back(hex.at((uch >> 4) & hex_mask));
                out.push_back(hex.at(uch & hex_mask));
            } else {
                out.push_back(static_cast<char>(uch));
            }
            break;
        }
    }
    out.push_back('"');
    return out;
}

void append_string_list(
    std::ostringstream &out,
    const std::vector<std::string> &values) {
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << yaml_quote(values[i]);
    }
    out << "]";
}

void append_sequence_header(
    std::ostringstream &out,
    const std::string &key,
    bool empty,
    int depth) {
    append_indent(out, depth);
    out << key << ":";
    if (empty) {
        out << " []\n";
        return;
    }
    out << "\n";
}

void append_value_spec_yaml(
    std::ostringstream &out,
    const ValueSpec &spec,
    int depth) {
    append_indent(out, depth);
    out << "value:\n";
    append_indent(out, depth + 1);
    out << "hint: " << yaml_quote(format_value_hint(spec.hint)) << "\n";
    if (!spec.choices.empty()) {
        append_indent(out, depth + 1);
        out << "choices: ";
        append_string_list(out, spec.choices);
        out << "\n";
    }
    if (!spec.command.empty()) {
        append_indent(out, depth + 1);
        out << "command: " << yaml_quote(spec.command) << "\n";
    }
}

void append_option_yaml(
    std::ostringstream &out,
    const OptionSpec &option,
    int depth) {
    append_indent(out, depth);
    out << "- flags: ";
    append_string_list(out, option.flags);
    out << "\n";
    if (!option.description.empty()) {
        append_indent(out, depth + 1);
        out << "description: " << yaml_quote(option.description) << "\n";
    }
    append_indent(out, depth + 1);
    out << "takes_value: " << (option.takes_value ? "true" : "false") << "\n";
    append_indent(out, depth + 1);
    out << "repeatable: " << (option.repeatable ? "true" : "false") << "\n";
    if (option.takes_value) {
        append_value_spec_yaml(out, option.value, depth + 1);
    }
}

void append_arg_yaml(std::ostringstream &out, const ArgSpec &arg, int depth) {
    append_indent(out, depth);
    out << "- name: " << yaml_quote(arg.name) << "\n";
    if (!arg.description.empty()) {
        append_indent(out, depth + 1);
        out << "description: " << yaml_quote(arg.description) << "\n";
    }
    append_indent(out, depth + 1);
    out << "optional: " << (arg.optional ? "true" : "false") << "\n";
    append_indent(out, depth + 1);
    out << "repeatable: " << (arg.repeatable ? "true" : "false") << "\n";
    append_value_spec_yaml(out, arg.values, depth + 1);
}

void append_command_yaml(
    std::ostringstream &out,
    const CommandSpec &command,
    int depth,
    bool list_item) {
    append_indent(out, depth);
    const int child_depth = list_item ? depth + 1 : depth;
    if (list_item) {
        out << "- name: " << yaml_quote(command.name) << "\n";
    } else {
        out << "name: " << yaml_quote(command.name) << "\n";
    }
    if (!command.description.empty()) {
        append_indent(out, child_depth);
        out << "description: " << yaml_quote(command.description) << "\n";
    }
    append_sequence_header(
        out,
        "options",
        command.options.empty(),
        child_depth);
    if (!command.options.empty()) {
        for (const auto &option : command.options) {
            append_option_yaml(out, option, child_depth + 1);
        }
    }

    append_sequence_header(
        out,
        "positionals",
        command.positionals.empty(),
        child_depth);
    if (!command.positionals.empty()) {
        for (const auto &arg : command.positionals) {
            append_arg_yaml(out, arg, child_depth + 1);
        }
    }

    append_sequence_header(
        out,
        "subcommands",
        command.subcommands.empty(),
        child_depth);
    if (!command.subcommands.empty()) {
        for (const auto &subcommand : command.subcommands) {
            append_command_yaml(out, subcommand, child_depth + 1, true);
        }
    }
}

} // namespace

std::string render_spec(const CompletionSpec &spec) {
    std::ostringstream out;
    out << "root:\n";
    append_command_yaml(out, spec.root, 1, false);
    return out.str();
}
