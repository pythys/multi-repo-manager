#ifndef SRC_LIB_REPO_FACTORY_HPP_
#define SRC_LIB_REPO_FACTORY_HPP_

#include <memory>
#include "repo_manager.hpp"
#include "tree.hpp"

std::unique_ptr<RepoManager> createRepoManager(RepoType type);

#endif  // SRC_LIB_REPO_FACTORY_HPP_
