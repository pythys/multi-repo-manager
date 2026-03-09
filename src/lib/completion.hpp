#ifndef SRC_LIB_COMPLETION_HPP_
#define SRC_LIB_COMPLETION_HPP_

#include <CLI/CLI.hpp>
#include <cstdint>
#include <string>
#include <vector>

enum class ShellType : std::uint8_t { Bash, Zsh, PowerShell };
enum class ValueHint : std::uint8_t { None, File, Dir, Enum, Command };

struct ValueSpec {
    ValueHint hint = ValueHint::None;
    std::vector<std::string> choices;
    std::string command;
};

struct ArgSpec {
    std::string name;
    bool optional = false;
    bool repeatable = false;
    ValueSpec values;
};

struct OptionSpec {
    std::vector<std::string> flags;
    bool takes_value = false;
    bool repeatable = false;
    ValueSpec value;
};

struct CommandSpec {
    std::string name;
    std::vector<OptionSpec> options;
    std::vector<ArgSpec> positionals;
    std::vector<CommandSpec> subcommands;
};

struct CompletionSpec {
    CommandSpec root;
};

ShellType parse_shell_type(const std::string &shell);
CompletionSpec extract_spec(CLI::App &app);
std::string print_spec(CLI::App &app);
std::string generate_script(CLI::App &app, ShellType shell);

#endif // SRC_LIB_COMPLETION_HPP_
