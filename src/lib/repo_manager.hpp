#ifndef SRC_LIB_REPO_MANAGER_HPP_
#define SRC_LIB_REPO_MANAGER_HPP_

/**
 * @file repo_manager.hpp
 * @brief Repository operations abstraction.
 */

#include "tree.hpp"
#include <cstdint>
#include <string>
#include <vector>

/** Sync status between branches. */
enum class BranchSyncState : std::uint8_t {
    UP_TO_DATE,
    TARGET_AHEAD,
    DIVERGED,
    SOURCE_AHEAD,
};

/**
 * @brief Backend-neutral repository manager interface.
 */
class RepoManager {
  public:
    /** Returns true when path points to a supported repository. */
    virtual bool is_repo(const std::string &path) = 0;

    /** Clone/copy a repository from source URL/path to destination path. */
    virtual void
    copy(const std::string &source, const std::string &destination) = 0;

    /** Add a remote to repository. */
    virtual void add_remote(const std::string &path, const Remote &remote) = 0;

    /** Remove a remote from repository. */
    virtual void
    remove_remote(const std::string &path, const Remote &remote) = 0;

    /** List remotes configured in repository. */
    virtual std::vector<Remote> get_remotes(const std::string &path) = 0;

    /** Create a local branch from configured remote branch. */
    virtual void add_branch(const std::string &path, const Branch &branch) = 0;

    /** Remove a local branch if safe. */
    virtual void
    remove_branch(const std::string &path, const Branch &branch) = 0;

    /** List tracked local branches and their upstream remotes. */
    virtual std::vector<Branch> get_branches(const std::string &path) = 0;

    /** Returns true if the local branch exists in repository. */
    virtual bool
    branch_exists(const std::string &path, const std::string &branch_name) = 0;

    /** Switch to a local branch. */
    virtual void
    switch_branch(const std::string &path, const std::string &branch_name) = 0;

    /** Pull remote branch into a local branch. */
    virtual void pull_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &remote_branch,
        const std::string &local_branch) = 0;

    /** Push a local branch to a remote branch. */
    virtual void push_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &local_branch,
        const std::string &remote_branch) = 0;

    /**
     * Compare branches and classify synchronization state.
     *
     * @param path Repository path.
     * @param source_branch Local branch compared from.
     * @param target_branch Local branch compared to.
     */
    virtual BranchSyncState compare_branches(
        const std::string &path,
        const std::string &source_branch,
        const std::string &target_branch) = 0;

    /** Return status lines similar to default `git status` semantics. */
    virtual std::vector<std::string> get_status(const std::string &path) = 0;

    virtual ~RepoManager() = default;
};

#endif // SRC_LIB_REPO_MANAGER_HPP_
