#include "completion_bash.hpp"
#include "utils.hpp"
#include <sstream>

namespace {

std::string escape_double_quotes(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
        case '\\':
        case '"':
        case '$':
        case '`':
            out.push_back('\\');
            out.push_back(ch);
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
            out.push_back(ch);
            break;
        }
    }
    return out;
}

std::string join_words(const std::vector<std::string> &values) {
    std::vector<std::string> escaped;
    escaped.reserve(values.size());
    for (const auto &value : values) {
        escaped.push_back(escape_double_quotes(value));
    }
    return join(escaped, " ");
}

std::string join_flags(const std::vector<std::string> &flags) {
    return join(flags, " ");
}

std::string join_flags_case(const std::vector<std::string> &flags) {
    return join(flags, "|");
}

const ArgSpec *first_positional(const CommandSpec &command) {
    if (command.positionals.empty()) {
        return nullptr;
    }
    return &command.positionals.front();
}

void append_value_completion(
    std::ostringstream &out,
    const ValueSpec &value,
    const std::string &indent) {
    switch (value.hint) {
    case ValueHint::File:
        out << indent << "COMPREPLY=( $(compgen -f -- \"$cur\") )\n";
        out << indent << "return\n";
        break;
    case ValueHint::Dir:
        out << indent << "COMPREPLY=( $(compgen -d -- \"$cur\") )\n";
        out << indent << "return\n";
        break;
    case ValueHint::Enum:
        out << indent << "COMPREPLY=( $(compgen -W \"";
        out << join_words(value.choices);
        out << "\" -- \"$cur\") )\n";
        out << indent << "return\n";
        break;
    case ValueHint::Command:
        out << indent << "COMPREPLY=( $(compgen -c -- \"$cur\") )\n";
        out << indent << "return\n";
        break;
    case ValueHint::None:
        break;
    }
}

void append_option_value_cases(
    std::ostringstream &out,
    const std::vector<OptionSpec> &options,
    const std::string &indent) {
    bool opened = false;
    for (const auto &option : options) {
        if (!option.takes_value || option.flags.empty()) {
            continue;
        }
        if (option.value.hint == ValueHint::None) {
            continue;
        }
        if (!opened) {
            out << indent << "case \"$prev\" in\n";
            opened = true;
        }
        out << indent << "  " << join_flags_case(option.flags) << ")\n";
        append_value_completion(out, option.value, indent + "    ");
        out << indent << "    ;;\n";
    }
    if (opened) {
        out << indent << "esac\n\n";
    }
}

void append_positional_completion(
    std::ostringstream &out,
    const CommandSpec &command,
    const std::string &indent) {
    const ArgSpec *arg = first_positional(command);
    if (arg == nullptr) {
        return;
    }
    if (arg->values.hint == ValueHint::None) {
        return;
    }
    out << indent << "if [[ $COMP_CWORD -eq 2 ]]; then\n";
    append_value_completion(out, arg->values, indent + "  ");
    out << indent << "fi\n\n";
}

} // namespace

std::string render_bash(const CompletionSpec &spec) {
    std::ostringstream out;

    out << "_" << spec.root.name << "_completion() {\n";
    out << "    local cur prev cmd opts\n";
    out << "    COMPREPLY=()\n";
    out << "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n";
    out << "    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n";
    out << "    cmd=\"${COMP_WORDS[1]}\"\n\n";

    std::vector<std::string> commands;
    commands.reserve(spec.root.subcommands.size());
    for (const auto &subcommand : spec.root.subcommands) {
        commands.push_back(subcommand.name);
    }

    const std::string root_opts = join_flags(
        spec.root.options.empty() ? std::vector<std::string>{} : [&]() {
            std::vector<std::string> flags;
            for (const auto &opt : spec.root.options) {
                flags.insert(flags.end(), opt.flags.begin(), opt.flags.end());
            }
            return flags;
        }());

    out << "    if [[ $COMP_CWORD -eq 1 ]]; then\n";
    out << "        if [[ $cur == -* ]]; then\n";
    out << "            COMPREPLY=( $(compgen -W \"";
    out << escape_double_quotes(root_opts);
    out << "\" -- \"$cur\") )\n";
    out << "        else\n";
    out << "            COMPREPLY=( $(compgen -W \"";
    out << join_words(commands);
    out << "\" -- \"$cur\") )\n";
    out << "        fi\n";
    out << "        return\n";
    out << "    fi\n\n";

    out << "    opts=\"\"\n\n";
    out << "    case \"$cmd\" in\n";
    for (const auto &subcommand : spec.root.subcommands) {
        out << "        " << subcommand.name << ")\n";
        std::vector<std::string> flags;
        for (const auto &option : subcommand.options) {
            flags.insert(flags.end(), option.flags.begin(), option.flags.end());
        }
        out << "            opts=\"" << escape_double_quotes(join_flags(flags))
            << "\"\n\n";
        append_option_value_cases(out, subcommand.options, "            ");
        append_positional_completion(out, subcommand, "            ");
        out << "            ;;\n";
    }
    out << "    esac\n\n";

    out << "    COMPREPLY=( $(compgen -W \"$opts\" -- \"$cur\") )\n";
    out << "}\n\n";
    out << "complete -F _" << spec.root.name << "_completion " << spec.root.name
        << "\n";

    return out.str();
}
