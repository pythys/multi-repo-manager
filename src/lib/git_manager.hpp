#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <string>
#include <utility>
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

    void update(const std::string& path) override {
        git_repository* repo = nullptr;
        int error = git_repository_open(&repo, path.c_str());
        if (error != 0) {
            const git_error* err = git_error_last();
            throw std::runtime_error(
                std::string("Failed to open repository: ") +
                (err ? err->message : "unknown error"));
        }

        git_reference* head_ref = nullptr;
        error = git_repository_head(&head_ref, repo);
        if (error != 0) {
            const git_error* err = git_error_last();
            git_repository_free(repo);
            throw std::runtime_error(
                std::string("Failed to retrieve HEAD: ") +
                (err ? err->message : "unknown error"));
        }

        const char* branch_name = nullptr;
        error = git_branch_name(&branch_name, head_ref);
        if (error != 0) {
            git_reference_free(head_ref);
            git_repository_free(repo);
            throw std::runtime_error("Failed to determine branch name");
        }

        git_reference* upstream_branch = nullptr;
        error = git_branch_upstream(&upstream_branch, head_ref);
        if (error != 0) {
            git_reference_free(head_ref);
            git_repository_free(repo);
            return;
        }

        git_reference_free(upstream_branch);

        git_buf remote_name_buf = GIT_BUF_INIT_CONST(nullptr, 0);
        error = git_branch_remote_name(&remote_name_buf, repo, branch_name);
        if (error != 0) {
            git_reference_free(head_ref);
            git_repository_free(repo);
            throw std::runtime_error("Failed to retrieve remote name");
        }

        const char* remote_name = remote_name_buf.ptr;

        git_remote* remote = nullptr;
        error = git_remote_lookup(&remote, repo, remote_name);
        if (error != 0) {
            git_buf_dispose(&remote_name_buf);
            git_reference_free(head_ref);
            git_repository_free(repo);
            throw std::runtime_error("Failed to lookup remote");
        }

        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        git_remote_callbacks remote_callbacks;
        git_remote_init_callbacks(
            &remote_callbacks,
            GIT_REMOTE_CALLBACKS_VERSION);
        remote_callbacks.credentials = GitManager::credential_callback;
        fetch_opts.callbacks = remote_callbacks;

        error = git_remote_fetch(remote, nullptr, &fetch_opts, nullptr);
        if (error != 0) {
            const git_error* err = git_error_last();
            git_remote_free(remote);
            git_buf_dispose(&remote_name_buf);
            git_reference_free(head_ref);
            git_repository_free(repo);
            throw std::runtime_error(
                std::string("Failed to fetch from remote: ") +
                (err ? err->message : "unknown error"));
        }

        git_remote_free(remote);

        git_merge_options merge_opts;
        git_merge_options_init(&merge_opts, GIT_MERGE_OPTIONS_VERSION);
        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

        git_annotated_commit* remote_commit = nullptr;
        error = git_annotated_commit_from_ref(&remote_commit, repo, head_ref);
        if (error == 0) {
            git_merge(repo, (const git_annotated_commit**)&remote_commit, 1,
                      &merge_opts, &checkout_opts);
            git_annotated_commit_free(remote_commit);
        }

        git_buf_dispose(&remote_name_buf);
        git_reference_free(head_ref);
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

    std::vector<std::string> get_status(const std::string& path) override {
        std::vector<std::string> status_lines;

        git_repository* repo = nullptr;
        int repo_open_result = git_repository_open(&repo, path.c_str());
        if (repo_open_result != 0) {
            const git_error* err = git_error_last();
            std::string error_message = "Failed to open repository: ";
            error_message += err ? err->message : "unknown error";
            status_lines.emplace_back(std::move(error_message));
            return status_lines;
        }

        git_status_options status_opts;
        git_status_options_init(&status_opts, GIT_STATUS_OPTIONS_VERSION);
        status_opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
        status_opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                            GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

        git_status_list* status_list = nullptr;
        int status_result = git_status_list_new(
            &status_list,
            repo,
            &status_opts);
        if (status_result != 0) {
            const git_error* err = git_error_last();
            std::string error_message = "Failed to retrieve status: ";
            error_message += err ? err->message : "unknown error";
            status_lines.emplace_back(std::move(error_message));
            git_repository_free(repo);
            return status_lines;
        }

        size_t entry_count = git_status_list_entrycount(status_list);

        auto transform_status = [&status_lines](const git_status_entry* entry) {
            if (entry->status & GIT_STATUS_INDEX_NEW) {
                std::string msg = "New file staged: ";
                msg += entry->head_to_index->new_file.path;
                status_lines.emplace_back(std::move(msg));
            }
            if (entry->status & GIT_STATUS_INDEX_MODIFIED) {
                std::string msg = "Modified file staged: ";
                msg += entry->head_to_index->new_file.path;
                status_lines.emplace_back(std::move(msg));
            }
            if (entry->status & GIT_STATUS_INDEX_DELETED) {
                std::string msg = "Deleted file staged: ";
                msg += entry->head_to_index->old_file.path;
                status_lines.emplace_back(std::move(msg));
            }
            if (entry->status & GIT_STATUS_WT_NEW) {
                std::string msg = "New file: ";
                msg += entry->index_to_workdir->new_file.path;
                status_lines.emplace_back(std::move(msg));
            }
            if (entry->status & GIT_STATUS_WT_MODIFIED) {
                std::string msg = "Modified file: ";
                msg += entry->index_to_workdir->new_file.path;
                status_lines.emplace_back(std::move(msg));
            }
            if (entry->status & GIT_STATUS_WT_DELETED) {
                std::string msg = "Deleted file: ";
                msg += entry->index_to_workdir->old_file.path;
                status_lines.emplace_back(std::move(msg));
            }
        };

        for (size_t i = 0; i < entry_count; ++i) {
            const git_status_entry* entry = git_status_byindex(status_list, i);
            if (entry) {
                transform_status(entry);
            }
        }

        git_status_list_free(status_list);
        git_repository_free(repo);

        if (status_lines.empty()) {
            status_lines.emplace_back("No changes detected");
        }

        return status_lines;
    }

    ~GitManager() override = default;
};

#endif  // SRC_LIB_GIT_MANAGER_HPP_
