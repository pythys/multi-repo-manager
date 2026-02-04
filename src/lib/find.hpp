#ifndef SRC_LIB_FIND_HPP_
#define SRC_LIB_FIND_HPP_

#include "tree.hpp"
#include <string>
#include <vector>

std::vector<Repo> find_repos(const std::string &path);

int run_find(const std::string &find_path, const std::string &save_path);

#endif // SRC_LIB_FIND_HPP_
