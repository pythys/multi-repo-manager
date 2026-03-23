#include "tree.hpp"

std::string repo_type_to_string(RepoType type) {
    switch (type) {
    case RepoType::GIT:
        return "git";
    case RepoType::SVN:
        return "svn";
    case RepoType::HG:
        return "hg";
    default:
        return "unknown";
    }
}
