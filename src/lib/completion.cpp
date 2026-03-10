#include "completion.hpp"
#include "completion_bash.hpp"
#include "completion_spec.hpp"
#include "completion_zsh.hpp"
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {

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
    if (lower.find("pattern") != std::string::npos) {
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
    spec.description = option.get_description();
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
    spec.description = option.get_description();
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
    spec.description = app.get_description();
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

CompletionSpec extract_spec(CLI::App &app) {
    CompletionSpec spec;
    spec.root = extract_command_spec(app);
    return spec;
}

std::string generate_script(CLI::App &app, const std::string &shell_type) {
    const CompletionSpec spec = extract_spec(app);
    if (shell_type == "spec") {
        return render_spec(spec);
    }
    if (shell_type == "zsh") {
        return render_zsh(spec);
    }
    if (shell_type == "bash") {
        return render_bash(spec);
    }
    if (shell_type == "powershell") {
        return render_spec(spec);
    }
    throw std::invalid_argument("unknown completion format: " + shell_type);
}
