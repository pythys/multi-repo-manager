#ifndef TREE_HPP
#define TREE_HPP

#include <string>
#include <vector>

enum class RemoteType { HTTPS, SSH, GIT };
enum class RepoType { GIT, SVN };

struct Remote {
    std::string name;
    std::string url;
    RemoteType type;
};

struct Repo {
    std::string name;
    RepoType type;
    std::vector<Remote> remotes;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif
