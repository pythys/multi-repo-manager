#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

#include <string>
#include <vector>

enum class RemoteType { HTTPS, SSH, GIT };
enum class RepoType { GIT, SVN };
enum class RepoStatus { PENDING, SYNCHING, SYNCHED };

struct Remote {
    std::string name;
    std::string url;
    RemoteType type;
};

struct Repo {
    std::string name;
    RepoType type;
    RepoStatus status;
    std::vector<Remote> remotes;
    std::vector<Repo> children;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif  // SRC_LIB_TREE_HPP_
