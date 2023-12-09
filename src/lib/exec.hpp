#ifndef SRC_LIB_EXEC_HPP_
#define SRC_LIB_EXEC_HPP_

#include <string>

int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type
);

#endif  // SRC_LIB_EXEC_HPP_
