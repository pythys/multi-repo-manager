#include <vector>
#include <iostream>
#include "git_manager.hpp"
#include "git2.h"

void GitManager::copy(
    const std::string& source,
    const std::string& destination) {

    git_repository* repo = nullptr;
    git_clone(&repo, source.c_str(), destination.c_str(), nullptr);
    git_repository_free(repo);
}

void GitManager::add_remote(
    const std::string& path,
    const Remote remote) {

    git_repository* repo = nullptr;
    git_repository_open(&repo, path.c_str());
    git_remote* gremote = nullptr;
    git_remote_create(&gremote, repo, remote.name.c_str(), remote.url.c_str());
    git_remote_free(gremote);
    git_repository_free(repo);
}

void GitManager::remove_remote(
    const std::string& path,
    const Remote remote) {

    git_repository* repo = nullptr;
    git_repository_open(&repo, path.c_str());
    git_remote_delete(repo, remote.name.c_str());
    git_repository_free(repo);
}

std::vector<Remote> GitManager::get_remotes(
    const std::string& path) {

    git_repository* repo = nullptr;
    git_repository_open(&repo, path.c_str());

    git_strarray gremotes = {nullptr, 0};
    git_remote_list(&gremotes, repo);

    std::vector<Remote> remotes = std::vector<Remote>();
    for (size_t i = 0; i < gremotes.count; i++) {
        git_remote *gremote = nullptr;
        git_remote_lookup(&gremote, repo, gremotes.strings[i]);
        const char *url = git_remote_url(gremote);

        Remote remote = Remote();
        remote.name = gremotes.strings[i];
        remote.url = url;
        remotes.push_back(remote);

        git_remote_free(gremote);
    }
    git_strarray_dispose(&gremotes);
    git_repository_free(repo);
    return remotes;
}
