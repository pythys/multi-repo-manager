#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include "git2.h"
#include "repo_manager.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

template <typename T, void (*free_resource)(T *)> class GitResource {
    T *resource_;

  public:
    explicit GitResource(T *resource = nullptr) : resource_(resource) {}
    ~GitResource() {
        free_resource(resource_);
    }
    T *get() const {
        return resource_;
    }
    T **get_address() {
        return &resource_;
    }
    void reset(T *resource = nullptr) {
        free_resource(resource_);
        resource_ = resource;
    }
    GitResource(const GitResource &) = delete;
    GitResource &operator=(const GitResource &) = delete;
};

using GitObject = GitResource<git_object, git_object_free>;
using GitReference = GitResource<git_reference, git_reference_free>;
using GitRemote = GitResource<git_remote, git_remote_free>;
using GitRepository = GitResource<git_repository, git_repository_free>;
using GitCommit = GitResource<git_commit, git_commit_free>;
using GitSignature = GitResource<git_signature, git_signature_free>;
using GitStatusList = GitResource<git_status_list, git_status_list_free>;
using GitBranchIterator =
    GitResource<git_branch_iterator, git_branch_iterator_free>;

class GitBuffer {
    git_buf buf_;

  public:
    GitBuffer() : buf_(GIT_BUF_INIT_CONST(NULL, 0)) {}
    ~GitBuffer() {
        git_buf_dispose(&buf_);
    }
    git_buf *get() {
        return &buf_;
    }
    const char *get_ptr() const {
        return buf_.ptr;
    }
};

class GitManager : public RepoManager {
  private:
    static void check_error(
        int error_code,
        const std::string &message,
        git_repository *repo = nullptr) {
        if (error_code != 0) {
            const git_error *err = git_error_last();
            const std::string error_msg =
                message + ": " + (err ? err->message : "unknown error");
            const bool should_cleanup =
                repo && git_repository_state(repo) != GIT_REPOSITORY_STATE_NONE;
            if (should_cleanup) {
                git_repository_state_cleanup(repo);
            }
            throw std::runtime_error(error_msg);
        }
    }

    static std::string delta_path(const git_diff_delta *delta) {
        if (!delta) {
            return "<unknown>";
        }
        const char *new_path = delta->new_file.path;
        const char *old_path = delta->old_file.path;
        if (new_path) {
            return {new_path};
        }
        if (old_path) {
            return {old_path};
        }
        return "<unknown>";
    }

    static void append_status_lines(
        const git_status_entry &entry,
        std::vector<std::string> &status_lines) {
        const git_diff_delta *head_to_index = entry.head_to_index;
        const git_diff_delta *index_to_workdir = entry.index_to_workdir;
        struct StatusRule {
            unsigned int flag;
            const char *label;
            const git_diff_delta *delta;
        };
        const std::array<StatusRule, 11> rules = {{
            {GIT_STATUS_INDEX_NEW, "New file staged: ", head_to_index},
            {GIT_STATUS_INDEX_MODIFIED,
             "Modified file staged: ",
             head_to_index},
            {GIT_STATUS_INDEX_DELETED, "Deleted file staged: ", head_to_index},
            {GIT_STATUS_INDEX_RENAMED, "Renamed file staged: ", head_to_index},
            {GIT_STATUS_INDEX_TYPECHANGE,
             "Type-changed file staged: ",
             head_to_index},
            {GIT_STATUS_WT_NEW, "New file: ", index_to_workdir},
            {GIT_STATUS_WT_MODIFIED, "Modified file: ", index_to_workdir},
            {GIT_STATUS_WT_DELETED, "Deleted file: ", index_to_workdir},
            {GIT_STATUS_WT_RENAMED, "Renamed file: ", index_to_workdir},
            {GIT_STATUS_WT_TYPECHANGE, "Type-changed file: ", index_to_workdir},
            {GIT_STATUS_CONFLICTED, "Conflicted file: ", index_to_workdir},
        }};
        for (const auto &rule : rules) {
            if (entry.status & rule.flag) {
                status_lines.emplace_back(
                    std::string(rule.label) + delta_path(rule.delta));
            }
        }
    }

    static int credential_callback(
        git_credential **out,
        const char *url,
        const char *username_from_url,
        unsigned int allowed_types,
        void *payload) {
        (void)url;
        (void)payload;
        if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
            if (git_credential_ssh_key_from_agent(out, username_from_url) ==
                0) {
                return 0;
            }
            const char *privatekey_path = "~/.ssh/id_ed25519";
            const char *publickey_path = "~/.ssh/id_ed25519.pub";
            return git_credential_ssh_key_new(
                out,
                username_from_url,
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
    GitManager() = default;
    ~GitManager() override = default;

    bool is_repo(const std::string &path) override {
        const bool path_exists = std::filesystem::exists(path);
        if (!path_exists) {
            return false;
        }
        GitRepository repo;
        const int error_code =
            git_repository_open(repo.get_address(), path.c_str());
        return error_code == 0;
    }

    void
    copy(const std::string &source, const std::string &destination) override {
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
            "Failed to clone repository in" + destination,
            repo.get());
    }

    void update(const std::string &path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference head_ref;
        check_error(
            git_repository_head(head_ref.get_address(), repo.get()),
            "Failed to retrieve HEAD in " + path,
            repo.get());

        GitReference upstream_ref;
        check_error(
            git_branch_upstream(upstream_ref.get_address(), head_ref.get()),
            "Failed to retrieve upstream reference in " + path,
            repo.get());

        GitBuffer remote_name_buf;
        check_error(
            git_branch_remote_name(
                remote_name_buf.get(),
                repo.get(),
                git_reference_name(upstream_ref.get())),
            "Failed to retrieve remote name in " + path,
            repo.get());

        GitRemote remote;
        check_error(
            git_remote_lookup(
                remote.get_address(),
                repo.get(),
                remote_name_buf.get_ptr()),
            "Failed to lookup remote in " + path,
            repo.get());

        GitSignature stash_signature;
        check_error(
            git_signature_now(
                stash_signature.get_address(),
                "mrm",
                "mrm@mrm.com"),
            "Failed to create signature",
            repo.get());
        git_oid stash_oid;
        const int stash_code = git_stash_save(
            &stash_oid,
            repo.get(),
            stash_signature.get(),
            "mrm pre-update stash",
            GIT_STASH_DEFAULT | GIT_STASH_INCLUDE_UNTRACKED);

        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks();
        check_error(
            git_remote_fetch(remote.get(), nullptr, &fetch_opts, nullptr),
            "Failed to fetch remote in " + path,
            repo.get());

        GitReference remote_ref;
        check_error(
            git_reference_lookup(
                remote_ref.get_address(),
                repo.get(),
                git_reference_name(upstream_ref.get())),
            "Failed to lookup remote reference in " + path,
            repo.get());

        const git_oid *target_oid = git_reference_target(remote_ref.get());
        GitReference new_head;
        check_error(
            git_reference_set_target(
                new_head.get_address(),
                head_ref.get(),
                target_oid,
                "Fast-forward"),
            "Failed to fast-forward update in " + path,
            repo.get());

        GitObject target_obj;
        std::array<char, GIT_OID_HEXSZ + 1> oid_str{};
        git_oid_tostr(oid_str.data(), oid_str.size(), target_oid);
        check_error(
            git_revparse_single(
                target_obj.get_address(),
                repo.get(),
                oid_str.data()),
            "Failed to lookup target commit in " + path,
            repo.get());
        check_error(
            git_reset(repo.get(), target_obj.get(), GIT_RESET_MIXED, nullptr),
            "Failed to reset index in " + path,
            repo.get());

        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy =
            GIT_CHECKOUT_FORCE | GIT_CHECKOUT_REMOVE_UNTRACKED;
        check_error(
            git_checkout_head(repo.get(), &checkout_opts),
            "Failed to update working directory in " + path,
            repo.get());

        if (stash_code != GIT_ENOTFOUND) {
            check_error(
                git_stash_pop(repo.get(), 0, nullptr),
                "Failed to pop stash in " + path,
                repo.get());
        }
    }

    void add_remote(const std::string &path, const Remote &remote) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitRemote gremote;
        check_error(
            git_remote_create(
                gremote.get_address(),
                repo.get(),
                remote.name.c_str(),
                remote.url.c_str()),
            "Failed to add remote in " + path,
            repo.get());
    }

    void remove_remote(const std::string &path, const Remote &remote) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        check_error(
            git_remote_delete(repo.get(), remote.name.c_str()),
            "Failed to remove remote in " + path,
            repo.get());
    }

    std::vector<Remote> get_remotes(const std::string &path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        git_strarray remote_names;
        check_error(
            git_remote_list(&remote_names, repo.get()),
            "Failed to list remotes in " + path,
            repo.get());

        std::vector<Remote> remotes;
        auto names = std::span(remote_names.strings, remote_names.count);
        for (const char *remote_name : names) {
            GitRemote remote;
            check_error(
                git_remote_lookup(
                    remote.get_address(),
                    repo.get(),
                    remote_name),
                "Failed to lookup remote in " + path,
                repo.get());

            Remote r;
            r.name = remote_name;
            r.url = git_remote_url(remote.get());
            remotes.push_back(r);
        }
        git_strarray_dispose(&remote_names);
        return remotes;
    }

    std::vector<Branch> get_branches(const std::string &path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());
        GitBranchIterator branch_iter;
        check_error(
            git_branch_iterator_new(
                branch_iter.get_address(),
                repo.get(),
                GIT_BRANCH_LOCAL),
            "Failed to create branch iterator in " + path,
            repo.get());
        std::vector<Branch> branches;
        while (true) {
            GitReference next_branch;
            git_branch_t branch_type = GIT_BRANCH_LOCAL;
            const int code = git_branch_next(
                next_branch.get_address(),
                &branch_type,
                branch_iter.get());
            if (code == GIT_ITEROVER) {
                break;
            }
            if (code != 0) {
                check_error(
                    code,
                    "Failed to iterate branches in " + path,
                    repo.get());
            }
            const char *name = nullptr;
            check_error(
                git_branch_name(&name, next_branch.get()),
                "Failed to get branch name in " + path,
                repo.get());
            const std::string branch_name(name);
            GitBuffer remote_name_buf;
            const char *next_ref = git_reference_name(next_branch.get());
            const int rcode = git_branch_upstream_remote(
                remote_name_buf.get(),
                repo.get(),
                next_ref);
            const bool is_tracked = rcode == 0;
            if (!is_tracked) {
                continue;
            }
            const std::string remote_name = remote_name_buf.get_ptr();
            GitReference head_ref;
            check_error(
                git_repository_head(head_ref.get_address(), repo.get()),
                "Failed to retrieve HEAD in " + path,
                repo.get());
            const bool is_current =
                git_reference_cmp(head_ref.get(), next_branch.get()) == 0;
            branches.push_back(
                Branch{
                    .name = branch_name,
                    .remote = remote_name,
                    .is_current = is_current});
        }
        return branches;
    }

    void add_branch(const std::string &path, const Branch &branch) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        if (branch.remote.empty()) {
            throw std::runtime_error(
                "Branch remote is missing for " + branch.name);
        }

        const std::string remote_ref_name =
            "refs/remotes/" + branch.remote + "/" + branch.name;
        GitReference remote_ref;
        check_error(
            git_reference_lookup(
                remote_ref.get_address(),
                repo.get(),
                remote_ref_name.c_str()),
            "Failed to lookup remote branch in " + path,
            repo.get());

        const git_oid *target_oid = git_reference_target(remote_ref.get());
        if (!target_oid) {
            throw std::runtime_error(
                "Remote branch target not found in " + path);
        }

        GitCommit target_commit;
        check_error(
            git_commit_lookup(
                target_commit.get_address(),
                repo.get(),
                target_oid),
            "Failed to lookup target commit in " + path,
            repo.get());

        GitReference new_branch;
        check_error(
            git_branch_create(
                new_branch.get_address(),
                repo.get(),
                branch.name.c_str(),
                target_commit.get(),
                0),
            "Failed to create branch in " + path,
            repo.get());

        const std::string upstream_name = branch.remote + "/" + branch.name;
        check_error(
            git_branch_set_upstream(new_branch.get(), upstream_name.c_str()),
            "Failed to set branch upstream in " + path,
            repo.get());
    }

    void remove_branch(const std::string &path, const Branch &branch) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference branch_ref;
        const int lookup_code = git_branch_lookup(
            branch_ref.get_address(),
            repo.get(),
            branch.name.c_str(),
            GIT_BRANCH_LOCAL);
        if (lookup_code == GIT_ENOTFOUND) {
            return;
        }
        check_error(
            lookup_code,
            "Failed to lookup branch in " + path,
            repo.get());

        GitReference head_ref;
        check_error(
            git_repository_head(head_ref.get_address(), repo.get()),
            "Failed to retrieve HEAD in " + path,
            repo.get());

        if (git_reference_cmp(head_ref.get(), branch_ref.get()) == 0) {
            return;
        }

        check_error(
            git_branch_delete(branch_ref.get()),
            "Failed to delete branch in " + path,
            repo.get());
    }

    void checkout_branch(
        const std::string &path,
        const std::string &branch_name) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference branch_ref;
        check_error(
            git_branch_lookup(
                branch_ref.get_address(),
                repo.get(),
                branch_name.c_str(),
                GIT_BRANCH_LOCAL),
            "Failed to lookup branch in " + path,
            repo.get());

        GitObject target_obj;
        check_error(
            git_reference_peel(
                target_obj.get_address(),
                branch_ref.get(),
                GIT_OBJECT_COMMIT),
            "Failed to peel branch reference in " + path,
            repo.get());

        check_error(
            git_repository_set_head(
                repo.get(),
                git_reference_name(branch_ref.get())),
            "Failed to set HEAD in " + path,
            repo.get());

        git_checkout_options checkout_opts;
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
        checkout_opts.checkout_strategy =
            GIT_CHECKOUT_SAFE | GIT_CHECKOUT_RECREATE_MISSING;
        check_error(
            git_checkout_head(repo.get(), &checkout_opts),
            "Failed to checkout branch in " + path,
            repo.get());
    }

    void fetch_remote(const std::string &path, const std::string &remote_name)
        override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitRemote remote;
        check_error(
            git_remote_lookup(
                remote.get_address(),
                repo.get(),
                remote_name.c_str()),
            "Failed to lookup remote in " + path,
            repo.get());

        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks();
        check_error(
            git_remote_fetch(remote.get(), nullptr, &fetch_opts, nullptr),
            "Failed to fetch remote in " + path,
            repo.get());
    }

    std::vector<std::string> get_status(const std::string &path) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        git_status_options status_opts;
        git_status_options_init(&status_opts, GIT_STATUS_OPTIONS_VERSION);
        status_opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
        status_opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
                            GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
                            GIT_STATUS_OPT_RENAMES_INDEX_TO_WORKDIR;

        GitStatusList status_list;
        check_error(
            git_status_list_new(
                status_list.get_address(),
                repo.get(),
                &status_opts),
            "Failed to retrieve status in " + path,
            repo.get());

        const size_t count = git_status_list_entrycount(status_list.get());
        std::vector<std::string> status_lines;
        for (size_t i = 0; i < count; ++i) {
            const git_status_entry *entry =
                git_status_byindex(status_list.get(), i);
            if (!entry) {
                continue;
            }
            append_status_lines(*entry, status_lines);
        }

        if (status_lines.empty()) {
            status_lines.emplace_back("No changes detected");
        }
        return status_lines;
    }
};

#endif // SRC_LIB_GIT_MANAGER_HPP_
