#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <string>
#include <vector>
#include "repo_manager.hpp"
#include "git2.h"

class GitManager : public RepoManager {
 public:
    static int credential_callback(
        git_credential** out,
        const char* url,
        const char* username_from_url,
        unsigned int allowed_types,
        void* payload) {
        (void)url;
        (void)payload;
        if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
            if (git_credential_ssh_key_from_agent(
                    out,
                    username_from_url) == 0) {
                return 0;
            }
        }
        if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
            const char* privatekey_path = "~/.ssh/id_rsa";
            const char* publickey_path = "~/.ssh/id_rsa.pub";

            return git_credential_ssh_key_new(
                out,
                username_from_url,
                publickey_path,
                privatekey_path,
                nullptr);
        }
        return GIT_PASSTHROUGH;
    }

    void copy(
        const std::string& source,
        const std::string& destination) override {
        git_repository* repo = nullptr;
        git_clone_options clone_opts;
        git_clone_options_init(&clone_opts, GIT_CLONE_OPTIONS_VERSION);
        clone_opts.bare = 0;
        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        clone_opts.checkout_opts = checkout_opts;
        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        git_remote_callbacks remote_callbacks;
        git_remote_init_callbacks(
            &remote_callbacks,
            GIT_REMOTE_CALLBACKS_VERSION);
        remote_callbacks.credentials = GitManager::credential_callback;
        fetch_opts.callbacks = remote_callbacks;
        clone_opts.fetch_opts = fetch_opts;
        int error = git_clone(
            &repo,
            source.c_str(),
            destination.c_str(),
            &clone_opts);
        if (error != 0) {
            const git_error* err = git_error_last();
            throw std::runtime_error(
                std::string("git_clone failed: ") +
                (err ? err->message : "unknown error"));
        }
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
