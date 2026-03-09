#ifndef SRC_LIB_REMOTESYNC_HPP_
#define SRC_LIB_REMOTESYNC_HPP_

#include <string>
#include <vector>

int run_remotesync(
    const std::string &config_file,
    const std::string &source_remote,
    const std::string &target_remote,
    const std::vector<std::string> &branches,
    bool dry_run = false,
    const std::vector<std::string> &root_patterns = {},
    int jobs = 0);

#endif // SRC_LIB_REMOTESYNC_HPP_
