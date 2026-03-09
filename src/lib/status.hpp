#ifndef SRC_LIB_STATUS_HPP_
#define SRC_LIB_STATUS_HPP_

#include <string>
#include <vector>

int run_status(
    const std::string &config_file,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_STATUS_HPP_
