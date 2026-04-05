#ifndef SRC_LIB_VCS_REPO_FACTORY_HPP_
#define SRC_LIB_VCS_REPO_FACTORY_HPP_

#include "core/tree.hpp"
#include "vcs/repo_manager.hpp"
#include <memory>

std::unique_ptr<RepoManager> create_repo_manager(RepoType type);

#endif // SRC_LIB_VCS_REPO_FACTORY_HPP_
