#ifndef SRC_LIB_REPO_MANAGER_HPP_
#define SRC_LIB_REPO_MANAGER_HPP_

#include "tree.hpp"
#include <cstdint>
#include <string>
#include <vector>

enum class BranchSyncState : std::uint8_t {
    UP_TO_DATE,
    TARGET_AHEAD,
    DIVERGED,
    SOURCE_AHEAD,
};

class RepoManager {
  public:
    virtual bool is_repo(const std::string &path) = 0;

    virtual void
    copy(const std::string &source, const std::string &destination) = 0;

    virtual void add_remote(const std::string &path, const Remote &remote) = 0;

    virtual void
    remove_remote(const std::string &path, const Remote &remote) = 0;

    virtual std::vector<Remote> get_remotes(const std::string &path) = 0;

    virtual void add_branch(const std::string &path, const Branch &branch) = 0;

    virtual void
    remove_branch(const std::string &path, const Branch &branch) = 0;

    virtual std::vector<Branch> get_branches(const std::string &path) = 0;

    virtual bool
    branch_exists(const std::string &path, const std::string &branch_name) = 0;

    virtual void
    switch_branch(const std::string &path, const std::string &branch_name) = 0;

    virtual void pull_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &remote_branch,
        const std::string &local_branch) = 0;

    virtual void push_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &local_branch,
        const std::string &remote_branch) = 0;

    virtual BranchSyncState compare_branches(
        const std::string &path,
        const std::string &source_branch,
        const std::string &target_branch) = 0;

    virtual std::vector<std::string> get_status(const std::string &path) = 0;

    virtual ~RepoManager() = default;
};

#endif // SRC_LIB_REPO_MANAGER_HPP_
