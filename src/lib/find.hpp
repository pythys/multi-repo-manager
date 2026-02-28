#ifndef SRC_LIB_FIND_HPP_
#define SRC_LIB_FIND_HPP_

/**
 * @file find.hpp
 * @brief Find command entry point and repository discovery helpers.
 */

#include "tree.hpp"
#include <string>
#include <vector>

/** Recursively discover repositories under path. */
std::vector<Repo> find_repos(const std::string &path);

/**
 * @brief Build config from discovered repos and print/save it.
 *
 * @return 0 on completion.
 */
int run_find(const std::string &find_path, const std::string &save_path);

#endif // SRC_LIB_FIND_HPP_
