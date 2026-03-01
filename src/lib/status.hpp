#ifndef SRC_LIB_STATUS_HPP_
#define SRC_LIB_STATUS_HPP_

/**
 * @file status.hpp
 * @brief Status command entry point.
 */

#include <string>
#include <vector>

/**
 * @brief Print repository status for all repos in configuration.
 *
 * @return 0 on completion.
 */
int run_status(
    const std::string &config_file,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_STATUS_HPP_
