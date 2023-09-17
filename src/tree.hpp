#include <string>
#include <vector>

struct Remote {
    std::string name;
    std::string url;
    std::string type;
};

struct Repo {
    std::string name;
    std::vector<Remote> remotes;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};
