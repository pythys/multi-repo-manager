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

/** Relation between target and source refs for remotesync decisions. */
enum class RefSyncState : std::uint8_t {
    UP_TO_DATE,
    TARGET_AHEAD,
    DIVERGED,
    SOURCE_AHEAD,
};

/**
 * @brief Backend-neutral repository manager interface.
 *
 * Implementations provide SCM-specific behavior.
 */
class RepoManager {
  public:
    /** Returns true when path points to a supported repository. */
    virtual bool is_repo(const std::string &path) = 0;

    /** Clone/copy a repository from source URL/path to destination path. */
    virtual void
    copy(const std::string &source, const std::string &destination) = 0;

    /** Update repository from its configured upstream. */
    virtual void update(const std::string &path) = 0;

    /** Add a remote to repository. */
    virtual void add_remote(const std::string &path, const Remote &remote) = 0;

    /** Remove a remote from repository. */
    virtual void
    remove_remote(const std::string &path, const Remote &remote) = 0;

    /** List remotes configured in repository. */
    virtual std::vector<Remote> get_remotes(const std::string &path) = 0;

    /** List tracked local branches and their upstream remotes. */
    virtual std::vector<Branch> get_branches(const std::string &path) = 0;

    /** Create a local branch from configured remote branch. */
    virtual void add_branch(const std::string &path, const Branch &branch) = 0;

    /** Remove a local branch if safe. */
    virtual void
    remove_branch(const std::string &path, const Branch &branch) = 0;

    /** Checkout a local branch. */
    virtual void checkout_branch(
        const std::string &path,
        const std::string &branch_name) = 0;

    /** Fetch a specific remote in repository. */
    virtual void
    fetch_remote(const std::string &path, const std::string &remote_name) = 0;

    /** Returns true if the reference exists in repository. */
    virtual bool
    ref_exists(const std::string &path, const std::string &ref_name) = 0;

    /**
     * Compare source and target refs and classify synchronization state.
     *
     * @param source_ref Candidate reference to push from.
     * @param target_ref Reference on target remote.
     */
    virtual RefSyncState compare_refs(
        const std::string &path,
        const std::string &source_ref,
        const std::string &target_ref) = 0;

    /**
     * Push source reference to target reference on a remote.
     *
     * @param remote_name Remote to push to.
     * @param source_ref Source reference (for example refs/heads/main).
     * @param target_ref Target reference (for example refs/heads/main).
     */
    virtual void push_ref(
        const std::string &path,
        const std::string &remote_name,
        const std::string &source_ref,
        const std::string &target_ref) = 0;

    /** Return status lines similar to default `git status` semantics. */
    virtual std::vector<std::string> get_status(const std::string &path) = 0;

    virtual ~RepoManager() = default;
};

#endif // SRC_LIB_REPO_MANAGER_HPP_
