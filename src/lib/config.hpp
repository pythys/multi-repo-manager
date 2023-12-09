#ifndef SRC_LIB_CONFIG_HPP_
#define SRC_LIB_CONFIG_HPP_

#include <string>
#include <vector>
#include "tree.hpp"

std::vector<Tree> get_config(const std::string& config_file);
std::vector<Tree> get_dependencies(const std::string& config_file);

#endif  // SRC_LIB_CONFIG_HPP_
