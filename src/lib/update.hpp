#ifndef SRC_LIB_UPDATE_HPP_
#define SRC_LIB_UPDATE_HPP_

/**
 * @file update.hpp
 * @brief Update command entry point.
 */

#include <string>

/**
 * @brief Update repositories in configuration from upstream.
 *
 * @return 0 on completion.
 */
int run_update(const std::string &config_file);

#endif // SRC_LIB_UPDATE_HPP_
