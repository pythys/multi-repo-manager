#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

#include <cstdint>
#include <string>
#include <vector>

enum class RepoType: std::uint8_t { GIT, SVN, HG };
enum class RepoStatus: std::uint8_t { PENDING, SYNCHING, SYNCHED };

struct Remote {
    std::string name;
    std::string url;
};

struct Repo {
    std::string name;
    RepoType type = RepoType::GIT;
    RepoStatus status = RepoStatus::PENDING;
    std::vector<Remote> remotes;
    std::vector<Repo> children;
    std::vector<std::string> messages;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif  // SRC_LIB_TREE_HPP_
