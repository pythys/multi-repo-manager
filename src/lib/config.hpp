#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "tree.hpp"
#include <string>
#include <vector>

std::vector<Tree> get_config(const std::string& config_file);
std::vector<Tree> get_dependencies(const std::string& config_file);

#endif
