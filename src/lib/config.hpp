#ifndef SRC_LIB_CONFIG_HPP_
#define SRC_LIB_CONFIG_HPP_

#include "tree.hpp"
#include <string>
#include <vector>

void write_config(
    const std::vector<Tree> &trees,
    const std::string &config_file);
std::string make_config(const std::vector<Tree> &trees);
std::vector<Tree> get_config(const std::string &config_file);
std::vector<Tree> get_dependencies(const std::string &config_file);

#endif // SRC_LIB_CONFIG_HPP_
