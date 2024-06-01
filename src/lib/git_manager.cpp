#include "repo_manager.hpp"
#include "git_manager.hpp"
#include "git2.h"

void GitManager::copy(
    const std::string& source,
    const std::string& destination) {

    git_repository* repo = nullptr;
    git_clone(&repo, source.c_str(), destination.c_str(), nullptr);
}
