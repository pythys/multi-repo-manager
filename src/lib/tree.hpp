#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

#include <cstdint>
#include <string>
#include <vector>

enum class RepoType: std::uint8_t { GIT, SVN, HG, UNKNOWN };
enum class RepoStatus: std::uint8_t { PENDING, SYNCHING, SYNCHED, UNKNOWN };

struct Remote {
    std::string name;
    std::string url;
};

struct Repo {
    std::string name;
    RepoType type = RepoType::UNKNOWN;
    RepoStatus status = RepoStatus::UNKNOWN;
    std::vector<Remote> remotes;
    std::vector<Repo> children;
    std::vector<std::string> messages;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif  // SRC_LIB_TREE_HPP_
