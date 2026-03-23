#ifndef SRC_LIB_FIND_HPP_
#define SRC_LIB_FIND_HPP_

#include "tree.hpp"
#include <string>
#include <vector>

std::string normalize_path(const std::string &path);

std::vector<Repo> find_repos(const std::string &path);

int run_find(
    const std::vector<std::string> &find_paths,
    const std::string &save_path);

#endif // SRC_LIB_FIND_HPP_
