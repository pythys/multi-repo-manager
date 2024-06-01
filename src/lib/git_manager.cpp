#include "repo_manager.hpp"
#include "git_manager.hpp"
#include "git2.h"

void GitManager::copy(
    const std::string& source,
    const std::string& destination) {

    git_repository* repo = nullptr;
    git_clone(&repo, source.c_str(), destination.c_str(), nullptr);
}

void GitManager::update(
    const std::string& path,
    const std::string remote_name) {

    git_repository* repo = nullptr;
    git_repository_open(&repo, path.c_str());
    git_fetch_options fetch_options = GIT_FETCH_OPTIONS_INIT;
    git_remote* remote = nullptr;
    git_remote_lookup(&remote, repo, remote_name.c_str());
    git_remote_fetch(remote, nullptr, &fetch_options, nullptr);
    git_remote_free(remote);
    git_repository_free(repo);
}
