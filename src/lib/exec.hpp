#ifndef SRC_LIB_EXEC_HPP_
#define SRC_LIB_EXEC_HPP_

/**
 * @file exec.hpp
 * @brief Exec command entry point.
 */

#include <string>

/**
 * @brief Execute custom command over configured repositories.
 *
 * @note Current implementation is a placeholder.
 * @return 0 on completion.
 */
int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type);

#endif // SRC_LIB_EXEC_HPP_
