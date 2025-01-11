#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include <algorithm>
#include <string>
#include <vector>
#include "repo_manager.hpp"
#include "git2.h"

template <typename T, void (*FreeFunc)(T*)>
class GitResource {
    T* resource_;
 public:
    explicit GitResource(T* resource = nullptr) : resource_(resource) {}
    ~GitResource() { FreeFunc(resource_); }
    T* get() const { return resource_; }
    T** get_address() { return &resource_; }
    void reset(T* resource = nullptr) {
        FreeFunc(resource_);
        resource_ = resource;
    }
    GitResource(const GitResource&) = delete;
    GitResource& operator=(const GitResource&) = delete;
};

using GitAnnotatedCommit = GitResource<git_annotated_commit,
                                       git_annotated_commit_free>;
using GitCommit = GitResource<git_commit, git_commit_free>;
using GitIndex = GitResource<git_index, git_index_free>;
using GitReference = GitResource<git_reference, git_reference_free>;
using GitRemote = GitResource<git_remote, git_remote_free>;
using GitRepository = GitResource<git_repository, git_repository_free>;
using GitStatusList = GitResource<git_status_list, git_status_list_free>;
using GitTree = GitResource<git_tree, git_tree_free>;

class GitBuffer {
    git_buf buf_;
 public:
    GitBuffer() : buf_(GIT_BUF_INIT_CONST(NULL, 0)) {}
    ~GitBuffer() { git_buf_dispose(&buf_); }
    git_buf* get() { return &buf_; }
    const char* get_ptr() const { return buf_.ptr; }
};

class GitManager : public RepoManager {
 private:
    static void check_error(int error_code, const std::string& message) {
        if (error_code != 0) {
            const git_error* err = git_error_last();
            throw std::runtime_error(message + ": " +
                (err ? err->message : "unknown error"));
        }
    }

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
                out, username_from_url,
                publickey_path,
                privatekey_path,
                nullptr);
        }
        return GIT_PASSTHROUGH;
    }

    static git_remote_callbacks create_remote_callbacks() {
        git_remote_callbacks callbacks;
        git_remote_init_callbacks(&callbacks, GIT_REMOTE_CALLBACKS_VERSION);
        callbacks.credentials = credential_callback;
        return callbacks;
    }

 public:
    GitManager() { }
    ~GitManager() override { }

    void copy(const std::string& source, const std::string& destination)
        override {
        GitRepository repo;
        git_clone_options clone_opts;
        git_clone_options_init(&clone_opts, GIT_CLONE_OPTIONS_VERSION);
        clone_opts.bare = 0;

        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
        clone_opts.checkout_opts = checkout_opts;

        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks();
        clone_opts.fetch_opts = fetch_opts;

        check_error(
            git_clone(
                repo.get_address(),
                source.c_str(),
                destination.c_str(),
                &clone_opts),
            "Failed to clone repository in" + destination);
    }

    void update(const std::string& path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path);

        GitReference head_ref;
        check_error(
            git_repository_head(head_ref.get_address(), repo.get()),
            "Failed to retrieve HEAD in " + path);

        GitReference upstream_ref;
        check_error(
            git_branch_upstream(upstream_ref.get_address(), head_ref.get()),
            "Failed to retrieve upstream reference in " + path);

        GitBuffer remote_name_buf;
        check_error(
            git_branch_remote_name(
                remote_name_buf.get(),
                repo.get(),
                git_reference_name(upstream_ref.get())),
            "Failed to retrieve remote name in " + path);

        GitRemote remote;
        check_error(
            git_remote_lookup(
                remote.get_address(),
                repo.get(),
                remote_name_buf.get_ptr()),
            "Failed to lookup remote in "+ path);

        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks();

        check_error(
            git_remote_fetch(
                remote.get(),
                nullptr,
                &fetch_opts,
                nullptr),
            "Failed to fetch from remote in " + path);

        GitAnnotatedCommit remote_commit;
        check_error(
            git_annotated_commit_from_ref(
                remote_commit.get_address(),
                repo.get(),
                upstream_ref.get()),
            "Failed to create annotated commit in " + path);

        git_merge_options merge_opts;
        git_merge_options_init(&merge_opts, GIT_MERGE_OPTIONS_VERSION);

        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

        check_error(
            git_merge(
                repo.get(),
                reinterpret_cast<const git_annotated_commit**>(&remote_commit),
                1,
                &merge_opts,
                &checkout_opts),
            "Failed to merge changes in " + path);

        if (git_repository_state(repo.get()) == GIT_REPOSITORY_STATE_MERGE) {
            check_error(
                git_repository_state_cleanup(repo.get()),
                "Failed to clean up repository state after merge in " + path);
        }
    }

    void add_remote(const std::string& path, Remote remote) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path);

        GitRemote gremote;
        check_error(
            git_remote_create(
                gremote.get_address(),
                repo.get(),
                remote.name.c_str(),
                remote.url.c_str()),
            "Failed to add remote in " + path);
    }

    void remove_remote(const std::string& path, Remote remote) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path);

        check_error(
            git_remote_delete(repo.get(), remote.name.c_str()),
            "Failed to remove remote in " + path);
    }

    std::vector<Remote> get_remotes(const std::string& path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path);

        git_strarray remote_names;
        check_error(
            git_remote_list(&remote_names, repo.get()),
            "Failed to list remotes in " + path);

        std::vector<Remote> remotes;
        for (size_t i = 0; i < remote_names.count; ++i) {
            const char* remote_name = remote_names.strings[i];

            GitRemote remote;
            check_error(
                git_remote_lookup(
                    remote.get_address(),
                    repo.get(),
                    remote_name),
                "Failed to lookup remote in " + path);

            Remote r;
            r.name = remote_name;
            r.url = git_remote_url(remote.get());
            remotes.push_back(r);
        }
        git_strarray_dispose(&remote_names);
        return remotes;
    }

    std::vector<std::string> get_status(const std::string& path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path);

        git_status_options status_opts;
        git_status_options_init(&status_opts, GIT_STATUS_OPTIONS_VERSION);
        status_opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
        status_opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                            GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX;

        GitStatusList status_list;
        check_error(
            git_status_list_new(
                status_list.get_address(),
                repo.get(),
                &status_opts),
            "Failed to retrieve status in " + path);

        size_t count = git_status_list_entrycount(status_list.get());
        std::vector<std::string> status_lines;
        for (size_t i = 0; i < count; ++i) {
            const git_status_entry* entry =
                git_status_byindex(status_list.get(), i);
            if (!entry) {
                continue;
            }

            if (entry->status & GIT_STATUS_INDEX_NEW) {
                status_lines.emplace_back(
                    "New file staged: " +
                    std::string(entry->head_to_index->new_file.path));
            }
            if (entry->status & GIT_STATUS_INDEX_MODIFIED) {
                status_lines.emplace_back(
                    "Modified file staged: " +
                    std::string(entry->head_to_index->new_file.path));
            }
            if (entry->status & GIT_STATUS_INDEX_DELETED) {
                status_lines.emplace_back(
                    "Deleted file staged: " +
                    std::string(entry->head_to_index->old_file.path));
            }
            if (entry->status & GIT_STATUS_WT_NEW) {
                status_lines.emplace_back(
                    "New file: " +
                    std::string(entry->index_to_workdir->new_file.path));
            }
            if (entry->status & GIT_STATUS_WT_MODIFIED) {
                status_lines.emplace_back(
                    "Modified file: " +
                    std::string(entry->index_to_workdir->new_file.path));
            }
            if (entry->status & GIT_STATUS_WT_DELETED) {
                status_lines.emplace_back(
                    "Deleted file: " +
                    std::string(entry->index_to_workdir->old_file.path));
            }
        }

        if (status_lines.empty()) {
            status_lines.emplace_back("No changes detected");
        }
        return status_lines;
    }
};

#endif  // SRC_LIB_GIT_MANAGER_HPP_
