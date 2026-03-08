#include "completion.hpp"
#include <iostream>
#include <unordered_map>

ShellType parse_shell_type(const std::string &shell) {
    static const std::unordered_map<std::string, ShellType> shell_map = {
        {"bash", ShellType::Bash},
        {"zsh", ShellType::Zsh},
        {"powershell", ShellType::PowerShell}};
    return shell_map.at(shell);
}

std::string generate_script(const CLI::App &app, ShellType shell) {
    (void)app;
    (void)shell;
    std::cout << "completion generation not implemented\n";
    return "";
}
