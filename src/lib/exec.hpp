#ifndef SRC_LIB_EXEC_HPP_
#define SRC_LIB_EXEC_HPP_

/**
 * @file exec.hpp
 * @brief Exec command entry point.
 */

#include <string>
#include <vector>

/**
 * @brief Execute custom command over configured repositories.
 *
 * Runs the command in each targeted repository. When the SCM CLI for the
 * repository type is available (for example, `git`), it is prepended unless
 * the command already starts with that CLI.
 *
 * @return 0 when all targeted repositories succeed, otherwise 1.
 */
int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_EXEC_HPP_
