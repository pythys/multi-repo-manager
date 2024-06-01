#ifndef SRC_LIB_REPO_MANAGER_HPP_
#define SRC_LIB_REPO_MANAGER_HPP_
#include <string>

class RepoManager {
 public:
    virtual void copy(
        const std::string& source,
        const std::string& destination) = 0;
    virtual void update(
        const std::string& path,
        const std::string remote_name) = 0;
    virtual ~RepoManager() = default;
};

#endif  // SRC_LIB_REPO_MANAGER_HPP_
