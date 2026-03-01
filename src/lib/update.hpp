#ifndef SRC_LIB_UPDATE_HPP_
#define SRC_LIB_UPDATE_HPP_

/**
 * @file update.hpp
 * @brief Update command entry point.
 */

#include <string>
#include <vector>

/**
 * @brief Update repositories in configuration from upstream.
 *
 * @return 0 on completion.
 */
int run_update(
    const std::string &config_file,
    int pool_size = 0,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_UPDATE_HPP_
