#include "completion_zsh.hpp"
#include "utils.hpp"
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool has_flag(const OptionSpec &option, const std::string &flag) {
    return std::ranges::any_of(option.flags, [&](const std::string &entry) {
        return entry == flag;
    });
}

bool is_help_option(const OptionSpec &option) {
    return has_flag(option, "--help") || has_flag(option, "-h");
}

std::string join_flags_space(const std::vector<std::string> &flags) {
    return join(flags, " ");
}

std::string join_flags_comma(const std::vector<std::string> &flags) {
    return join(flags, ",");
}

std::string value_action_from_hint(const ValueSpec &value) {
    switch (value.hint) {
    case ValueHint::File:
        return "_files";
    case ValueHint::Dir:
        return "_files -/";
    case ValueHint::Enum: {
        std::ostringstream out;
        out << "(";
        for (std::size_t i = 0; i < value.choices.size(); ++i) {
            if (i > 0) {
                out << " ";
            }
            out << value.choices[i];
        }
        out << ")";
        return out.str();
    }
    case ValueHint::Command:
    case ValueHint::None:
        return "";
    }
    return "";
}

std::string option_value_label(const OptionSpec &option) {
    if (!option.takes_value) {
        return "";
    }
    if (option.value.hint == ValueHint::File) {
        if (option.description == "config file") {
            return option.description;
        }
        return "file";
    }
    if (option.value.hint == ValueHint::Dir) {
        return "directory";
    }
    if (option.value.hint == ValueHint::Command) {
        return "command";
    }
    if (option.value.hint == ValueHint::None) {
        return "value";
    }
    return "";
}

std::string option_description(const OptionSpec &option) {
    if (is_help_option(option)) {
        return "show help";
    }
    return option.description;
}

std::string format_option_entry(const OptionSpec &option) {
    std::ostringstream out;
    if (option.flags.size() > 1) {
        out << "'(" << join_flags_space(option.flags) << ")'";
        out << "{" << join_flags_comma(option.flags) << "}";
        out << "'[" << option_description(option) << "]";
    } else {
        out << "'" << option.flags.front() << "[" << option_description(option)
            << "]";
    }

    if (option.takes_value) {
        const std::string label = option_value_label(option);
        const std::string action = value_action_from_hint(option.value);
        if (!label.empty() || !action.empty()) {
            out << ":";
            if (!label.empty()) {
                out << label;
                if (!action.empty()) {
                    out << ":" << action;
                }
            } else {
                out << action;
            }
        }
    }
    out << "'";
    return out.str();
}

std::string format_positional_entry(const ArgSpec &arg) {
    std::ostringstream out;
    out << "'1:" << arg.name;
    const std::string action = value_action_from_hint(arg.values);
    if (!action.empty()) {
        out << ":" << action;
    }
    out << "'";
    return out.str();
}

} // namespace

std::string render_zsh(const CompletionSpec &spec) {
    std::ostringstream out;
    out << "#compdef " << spec.root.name << "\n\n";
    out << "_" << spec.root.name << "() {\n";
    out << "  local context state state_descr line\n";
    out << "  typeset -A opt_args\n\n";
    out << "  _arguments -C \\\n";

    for (const auto &option : spec.root.options) {
        out << "    " << format_option_entry(option) << " \\\n";
    }
    out << "    '1:command:->cmd' \\\n";
    out << "    '*::arg:->args'\n\n";
    out << "  case $state in\n";
    out << "    cmd)\n";
    out << "      local -a subcmds\n";
    out << "      subcmds=(\n";
    for (const auto &subcommand : spec.root.subcommands) {
        const std::string desc = subcommand.description.empty()
                                     ? subcommand.name
                                     : subcommand.description;
        out << "        '" << subcommand.name << ":" << desc << "'\n";
    }
    out << "      )\n";
    out << "      zstyle ':completion:*:" << spec.root.name
        << ":*:values' sort false\n";
    out << "      _describe 'command' subcmds\n";
    out << "      ;;\n";
    out << "    args)\n";
    out << "      case $line[1] in\n";
    for (const auto &subcommand : spec.root.subcommands) {
        out << "        " << subcommand.name << ")\n";
        out << "          _arguments \\\n";
        std::vector<std::string> specs;
        specs.reserve(subcommand.options.size() + 1);
        for (const auto &option : subcommand.options) {
            specs.push_back(format_option_entry(option));
        }
        if (!subcommand.positionals.empty()) {
            specs.push_back(
                format_positional_entry(subcommand.positionals.front()));
        }
        for (std::size_t i = 0; i < specs.size(); ++i) {
            const bool last = i + 1 == specs.size();
            out << "            " << specs.at(i);
            if (!last) {
                out << " \\\n";
            } else {
                out << "\n";
            }
        }
        out << "          ;;\n";
    }
    out << "      esac\n";
    out << "      ;;\n";
    out << "  esac\n";
    out << "}\n\n";
    out << "compdef _" << spec.root.name << " " << spec.root.name << "\n";
    return out.str();
}
