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

std::vector<Tree> filter_trees_by_root(
    const std::vector<Tree> &trees,
    const std::vector<std::string> &root_patterns);

std::vector<Tree> filter_trees_by_name(
    const std::vector<Tree> &trees,
    const std::vector<std::string> &name_patterns);

std::vector<Tree> load_trees(
    const std::string &config_file,
    const std::vector<std::string> &find_paths,
    const std::vector<std::string> &root_patterns,
    const std::vector<std::string> &name_patterns);

#endif // SRC_LIB_CONFIG_HPP_
