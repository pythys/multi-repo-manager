#include "core/tree.hpp"
#include <algorithm>

const Branch *find_current_branch(const std::vector<Branch> &branches) {
    auto it = std::ranges::find_if(branches, [](const Branch &branch) {
        return branch.is_current;
    });
    return it == branches.end() ? nullptr : &(*it);
}
