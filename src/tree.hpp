#ifndef TREE_HPP
#define TREE_HPP

#include <string>
#include <vector>

struct Remote {
    std::string name;
    std::string url;
    std::string type;
};

struct Repo {
    std::string name;
    std::string type;
    std::vector<Remote> remotes;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

#endif
