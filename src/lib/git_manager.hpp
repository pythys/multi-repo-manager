#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <string>
#include <vector>
#include "repo_manager.hpp"

class GitManager : public RepoManager {
 public:
    void copy(
        const std::string& source,
        const std::string& destination) override;

    void add_remote(
        const std::string& path,
        const Remote remote) override;

    void remove_remote(
        const std::string& path,
        const Remote remote) override;

    std::vector<Remote> get_remotes(
        const std::string& path) override;

    ~GitManager() override = default;
};

#endif  // SRC_LIB_GIT_MANAGER_HPP_
