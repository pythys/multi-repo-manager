#ifndef SRC_LIB_REPO_MANAGER_HPP_
#define SRC_LIB_REPO_MANAGER_HPP_

#include <string>
#include <vector>
#include "tree.hpp"

class RepoManager {
 public:
    virtual bool is_repo(const std::string& path) = 0;

    virtual void copy(
        const std::string& source,
        const std::string& destination) = 0;

    virtual void update(
        const std::string& path) = 0;

    virtual void add_remote(
        const std::string& path,
        const Remote remote) = 0;

    virtual void remove_remote(
        const std::string& path,
        const Remote remote) = 0;

    virtual std::vector<Remote> get_remotes(
        const std::string& path) = 0;

    virtual std::vector<std::string> get_status(
        const std::string& path) = 0;

    virtual ~RepoManager() = default;
};

#endif  // SRC_LIB_REPO_MANAGER_HPP_
