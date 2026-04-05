#include "core/tree.hpp"
#include <algorithm>

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

const Branch *find_current_branch(const std::vector<Branch> &branches) {
    auto it = std::ranges::find_if(branches, [](const Branch &branch) {
        return branch.is_current;
    });
    return it == branches.end() ? nullptr : &(*it);
}
