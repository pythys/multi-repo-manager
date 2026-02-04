#include "exec.hpp"
#include <iostream>
#include <string>

int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type) {
    std::cout << custom_command + config_file + repo_type;
    return 0;
}
