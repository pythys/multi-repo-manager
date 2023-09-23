#ifndef TREE_HPP
#define TREE_HPP

#include <string>
#include <vector>

enum RemoteType { HTTPS, SSH };
enum RepoType { GIT, SVN };

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
