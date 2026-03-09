#ifndef SRC_LIB_UPDATE_HPP_
#define SRC_LIB_UPDATE_HPP_

#include <string>
#include <vector>

int run_update(
    const std::string &config_file,
    int pool_size = 0,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_UPDATE_HPP_
