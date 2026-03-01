#ifndef SRC_LIB_REMOTESYNC_HPP_
#define SRC_LIB_REMOTESYNC_HPP_

/**
 * @file remotesync.hpp
 * @brief Remote branch synchronization command entry point.
 */

#include <string>
#include <vector>

/**
 * @brief Sync selected branches from source remote/fallback to target remote.
 *
 * @return 0 on successful run, 1 when operational errors occur.
 */
int run_remotesync(
    const std::string &config_file,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::vector<std::string> &branches,
    bool dry_run = false,
    const std::vector<std::string> &root_patterns = {},
    int jobs = 0);

#endif // SRC_LIB_REMOTESYNC_HPP_
