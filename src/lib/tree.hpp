#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

#include <cstdint>
#include <string>
#include <vector>

enum class RepoType : std::uint8_t { GIT, SVN, HG, UNKNOWN };
enum class RepoPhase : std::uint8_t { QUEUED, RUNNING, SUCCEEDED, FAILED };

struct Remote {
    std::string name;
    std::string url;
};

struct Branch {
    std::string name;
    std::string remote;
    bool is_current;
};

struct Repo {
    std::string name;
    RepoType type = RepoType::UNKNOWN;
    RepoPhase phase = RepoPhase::QUEUED;
    std::vector<Remote> remotes;
    std::vector<Branch> branches;
    std::vector<Repo> children;
    std::vector<std::string> messages;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

std::string repo_type_to_string(RepoType type);

#endif // SRC_LIB_TREE_HPP_
