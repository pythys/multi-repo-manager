#ifndef SRC_LIB_COMPLETION_HPP_
#define SRC_LIB_COMPLETION_HPP_

#include <CLI/CLI.hpp>
#include <cstdint>
#include <string>

enum class ShellType : std::uint8_t { Bash, Zsh, PowerShell };

ShellType parse_shell_type(const std::string &shell);
std::string generate_script(const CLI::App &app, ShellType shell);

#endif // SRC_LIB_COMPLETION_HPP_
