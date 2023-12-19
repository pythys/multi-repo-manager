#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <string>
#include "repo_manager.hpp"

class GitManager : public RepoManager {
 public:
    void copy(
        const std::string& source,
        const std::string& destination) override;

    ~GitManager() override = default;
};

#endif  // SRC_LIB_GIT_MANAGER_HPP_
