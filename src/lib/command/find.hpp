#ifndef SRC_LIB_COMMAND_FIND_HPP_
#define SRC_LIB_COMMAND_FIND_HPP_

#include <string>
#include <vector>

int run_find(
    const std::vector<std::string> &find_paths,
    const std::string &save_path,
    int min_depth = 0);

#endif // SRC_LIB_COMMAND_FIND_HPP_
