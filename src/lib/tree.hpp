#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

/**
 * @file tree.hpp
 * @brief Core data model for configured repository trees and runtime state.
 */

#include <cstdint>
#include <string>
#include <vector>

/** Supported repository implementations. */
enum class RepoType : std::uint8_t { GIT, SVN, HG, UNKNOWN };
/** Runtime phase for a repo operation. */
enum class RepoPhase : std::uint8_t { QUEUED, RUNNING, SUCCEEDED, FAILED };

/** Remote endpoint configuration. */
struct Remote {
    std::string name;
    std::string url;
};

/** Branch configuration/state. */
struct Branch {
    std::string name;
    std::string remote;
    bool is_current;
};

/** Repository node within a tree. */
struct Repo {
    /** Path relative to tree root. */
    std::string name;
    RepoType type = RepoType::UNKNOWN;
    /** Last known execution phase for long-running commands. */
    RepoPhase phase = RepoPhase::QUEUED;
    std::vector<Remote> remotes;
    std::vector<Branch> branches;
    /** Nested repositories under this repo path. */
    std::vector<Repo> children;
    /** Collected informational/warning/error messages. */
    std::vector<std::string> messages;
};

/** Root workspace plus configured repositories. */
struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif // SRC_LIB_TREE_HPP_
