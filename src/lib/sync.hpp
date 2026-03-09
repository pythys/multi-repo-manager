#ifndef SRC_LIB_SYNC_HPP_
#define SRC_LIB_SYNC_HPP_

#include <string>
#include <vector>

int run_sync(
    const std::string &config_file,
    int pool_size = 0,
    bool prune_remotes = false,
    bool prune_branches = false,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_SYNC_HPP_
