#ifndef SRC_LIB_PERSISTENCE_DISCOVERY_HPP_
#define SRC_LIB_PERSISTENCE_DISCOVERY_HPP_

#include "core/tree.hpp"
#include <string>
#include <vector>

std::string normalize_path(const std::string &path);

std::vector<Repo> find_repos(const std::string &path, int min_depth = 0);

#endif // SRC_LIB_PERSISTENCE_DISCOVERY_HPP_
