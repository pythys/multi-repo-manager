#ifndef SRC_LIB_REPO_FACTORY_HPP_
#define SRC_LIB_REPO_FACTORY_HPP_

#include "repo_manager.hpp"
#include "tree.hpp"
#include <memory>

std::unique_ptr<RepoManager> create_repo_manager(RepoType type);

#endif // SRC_LIB_REPO_FACTORY_HPP_
