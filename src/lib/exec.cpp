#include <iostream>
#include <string>
#include "exec.hpp"

auto run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type) -> int {
    std::cout << custom_command + config_file + repo_type;
    return 0;
}
