#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <string>
#include <vector>
#include "repo_manager.hpp"
#include "git2.h"

class GitManager : public RepoManager {
 public:
    void copy(
        const std::string& source,
        const std::string& destination) override {
        git_repository* repo = nullptr;
        git_clone(&repo, source.c_str(), destination.c_str(), nullptr);
        git_repository_free(repo);
    }

    void add_remote(
        const std::string& path,
        const Remote remote) override {
        git_repository* repo = nullptr;
        git_repository_open(&repo, path.c_str());
        git_remote* gremote = nullptr;
        git_remote_create(
            &gremote,
            repo,
            remote.name.c_str(),
            remote.url.c_str());
        git_remote_free(gremote);
        git_repository_free(repo);
    }

    void remove_remote(
        const std::string& path,
        const Remote remote) override {
        git_repository* repo = nullptr;
        git_repository_open(&repo, path.c_str());
        git_remote_delete(repo, remote.name.c_str());
        git_repository_free(repo);
    }

    std::vector<Remote> get_remotes(
        const std::string& path) override {
        git_repository* repo = nullptr;
        git_repository_open(&repo, path.c_str());

        git_strarray gremotes = {nullptr, 0};
        git_remote_list(&gremotes, repo);

        std::vector<Remote> remotes;
        for (size_t i = 0; i < gremotes.count; i++) {
            git_remote* gremote = nullptr;
            git_remote_lookup(&gremote, repo, gremotes.strings[i]);
            const char* url = git_remote_url(gremote);

            Remote remote;
            remote.name = gremotes.strings[i];
            remote.url = url;
            remotes.push_back(remote);

            git_remote_free(gremote);
        }
        git_strarray_dispose(&gremotes);
        git_repository_free(repo);
        return remotes;
    }

    ~GitManager() override = default;
};

#endif  // SRC_LIB_GIT_MANAGER_HPP_
