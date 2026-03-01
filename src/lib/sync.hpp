#ifndef SRC_LIB_SYNC_HPP_
#define SRC_LIB_SYNC_HPP_

/**
 * @file sync.hpp
 * @brief Sync command entry point.
 */

#include <string>
#include <vector>

/**
 * @brief Sync repositories based on configuration file.
 *
 * @return 0 on completion.
 */
int run_sync(
    const std::string &config_file,
    int pool_size = 0,
    bool prune_remotes = false,
    bool prune_branches = false,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_SYNC_HPP_
