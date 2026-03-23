#ifndef SRC_LIB_GIT_MANAGER_HPP_
#define SRC_LIB_GIT_MANAGER_HPP_

#include "git2.h"
#include "repo_manager.hpp"
#include "runtime.hpp"
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
    struct SshKeyCursor {
        std::vector<std::filesystem::path> keys;
        size_t next_index = 0;
    };

    static std::vector<std::filesystem::path> discover_ssh_keys() {
        const auto home_dir = get_home_directory();
        if (!home_dir) {
            return {};
        }

        const std::filesystem::path ssh_dir = *home_dir / ".ssh";
        if (!std::filesystem::exists(ssh_dir)) {
            return {};
        }

        std::vector<std::filesystem::path> keys;
        try {
            for (const auto &entry :
                 std::filesystem::directory_iterator(ssh_dir)) {
                const auto status = entry.symlink_status();
                if (!std::filesystem::is_regular_file(status) &&
                    !std::filesystem::is_symlink(status)) {
                    continue;
                }

                const auto &path = entry.path();
                const std::string filename = path.filename().string();
                if (filename == "config" || filename == "authorized_keys" ||
                    filename.starts_with("known_hosts")) {
                    continue;
                }

                if (path.extension() == ".pub") {
                    continue;
                }

                keys.push_back(path);
            }
        } catch (const std::filesystem::filesystem_error &) {
            return {};
        }

        std::ranges::sort(keys);
        return keys;
    }

    static const std::vector<std::filesystem::path> &cached_ssh_keys() {
        static const std::vector<std::filesystem::path> keys =
            discover_ssh_keys();
        return keys;
    }

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
            {.flag = GIT_STATUS_INDEX_NEW,
             .label = "New file staged: ",
             .delta = head_to_index},
            {.flag = GIT_STATUS_INDEX_MODIFIED,
             .label = "Modified file staged: ",
             .delta = head_to_index},
            {.flag = GIT_STATUS_INDEX_DELETED,
             .label = "Deleted file staged: ",
             .delta = head_to_index},
            {.flag = GIT_STATUS_INDEX_RENAMED,
             .label = "Renamed file staged: ",
             .delta = head_to_index},
            {.flag = GIT_STATUS_INDEX_TYPECHANGE,
             .label = "Type-changed file staged: ",
             .delta = head_to_index},
            {.flag = GIT_STATUS_WT_NEW,
             .label = "New file: ",
             .delta = index_to_workdir},
            {.flag = GIT_STATUS_WT_MODIFIED,
             .label = "Modified file: ",
             .delta = index_to_workdir},
            {.flag = GIT_STATUS_WT_DELETED,
             .label = "Deleted file: ",
             .delta = index_to_workdir},
            {.flag = GIT_STATUS_WT_RENAMED,
             .label = "Renamed file: ",
             .delta = index_to_workdir},
            {.flag = GIT_STATUS_WT_TYPECHANGE,
             .label = "Type-changed file: ",
             .delta = index_to_workdir},
            {.flag = GIT_STATUS_CONFLICTED,
             .label = "Conflicted file: ",
             .delta = index_to_workdir},
        }};
        for (const auto &rule : rules) {
            if (entry.status & rule.flag) {
                std::string path = "<unknown>";
                if (rule.delta) {
                    if (rule.delta->new_file.path) {
                        path = rule.delta->new_file.path;
                    } else if (rule.delta->old_file.path) {
                        path = rule.delta->old_file.path;
                    }
                }
                status_lines.emplace_back(std::string(rule.label) + path);
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
        if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
            if (git_credential_ssh_key_from_agent(out, username_from_url) ==
                0) {
                return 0;
            }
            auto *state = static_cast<SshKeyCursor *>(payload);
            if (!state) {
                return GIT_PASSTHROUGH;
            }

            while (state->next_index < state->keys.size()) {
                const std::size_t key_index = state->next_index++;
                const std::filesystem::path private_key =
                    state->keys.at(key_index);
                const std::filesystem::path public_key =
                    private_key.string() + ".pub";
                const bool has_public_key = std::filesystem::exists(public_key);

                const char *publickey_path =
                    has_public_key ? public_key.c_str() : nullptr;
                const int code = git_credential_ssh_key_new(
                    out,
                    username_from_url,
                    publickey_path,
                    private_key.c_str(),
                    nullptr);
                if (code == 0) {
                    return 0;
                }
            }
            return GIT_PASSTHROUGH;
        }
        return GIT_PASSTHROUGH;
    }

    static git_remote_callbacks create_remote_callbacks(void *payload) {
        git_remote_callbacks callbacks;
        git_remote_init_callbacks(&callbacks, GIT_REMOTE_CALLBACKS_VERSION);
        callbacks.credentials = credential_callback;
        callbacks.payload = payload;
        return callbacks;
    }

    static git_oid lookup_ref_oid(git_repository *repo, const char *ref_name) {
        GitReference ref;
        check_error(
            git_reference_lookup(ref.get_address(), repo, ref_name),
            "Failed to lookup reference " + std::string(ref_name),
            repo);

        const git_oid *target = git_reference_target(ref.get());
        if (target != nullptr) {
            return *target;
        }

        GitReference resolved;
        check_error(
            git_reference_resolve(resolved.get_address(), ref.get()),
            "Failed to resolve symbolic reference " + std::string(ref_name),
            repo);

        target = git_reference_target(resolved.get());
        if (target == nullptr) {
            throw std::runtime_error(
                "Failed to resolve target oid for reference " +
                std::string(ref_name));
        }
        return *target;
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

        SshKeyCursor key_state{
            .keys = cached_ssh_keys(),
            .next_index = 0,
        };
        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks(&key_state);
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

    bool branch_exists(const std::string &path, const std::string &branch_name)
        override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference branch_ref;
        const int lookup_code = git_branch_lookup(
            branch_ref.get_address(),
            repo.get(),
            branch_name.c_str(),
            GIT_BRANCH_LOCAL);
        if (lookup_code == GIT_ENOTFOUND) {
            return false;
        }
        check_error(
            lookup_code,
            "Failed to lookup branch in " + path,
            repo.get());
        return true;
    }

    void switch_branch(const std::string &path, const std::string &branch_name)
        override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference branch_ref;
        const int lookup_code = git_branch_lookup(
            branch_ref.get_address(),
            repo.get(),
            branch_name.c_str(),
            GIT_BRANCH_LOCAL);
        if (lookup_code == GIT_ENOTFOUND) {
            throw std::runtime_error(
                "Missing branch " + branch_name + " in " + path);
        }
        check_error(
            lookup_code,
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

    void pull_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &remote_branch,
        const std::string &local_branch) override {
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

        GitReference local_ref;
        const int local_lookup = git_branch_lookup(
            local_ref.get_address(),
            repo.get(),
            local_branch.c_str(),
            GIT_BRANCH_LOCAL);
        const bool local_exists = local_lookup != GIT_ENOTFOUND;
        if (local_exists) {
            check_error(
                local_lookup,
                "Failed to lookup branch in " + path,
                repo.get());
        }

        bool is_current = false;
        if (local_exists) {
            GitReference head_ref;
            check_error(
                git_repository_head(head_ref.get_address(), repo.get()),
                "Failed to retrieve HEAD in " + path,
                repo.get());
            is_current =
                git_reference_cmp(head_ref.get(), local_ref.get()) == 0;
        }

        git_oid stash_oid{};
        int stash_code = GIT_ENOTFOUND;
        if (is_current) {
            GitSignature stash_signature;
            check_error(
                git_signature_now(
                    stash_signature.get_address(),
                    "mrm",
                    "mrm@mrm.com"),
                "Failed to create signature",
                repo.get());
            stash_code = git_stash_save(
                &stash_oid,
                repo.get(),
                stash_signature.get(),
                "mrm pre-pull stash",
                GIT_STASH_DEFAULT | GIT_STASH_INCLUDE_UNTRACKED);
        }

        const std::string remote_ref_name = "refs/heads/" + remote_branch;
        const std::string tracking_ref =
            "refs/remotes/" + remote_name + "/" + remote_branch;
        std::string refspec = remote_ref_name + ":" + tracking_ref;
        std::vector<char> refspec_buffer(refspec.begin(), refspec.end());
        refspec_buffer.push_back('\0');
        std::vector<char *> strings = {refspec_buffer.data()};
        git_strarray refspecs = {
            .strings = strings.data(),
            .count = strings.size()};

        SshKeyCursor key_state{
            .keys = cached_ssh_keys(),
            .next_index = 0,
        };
        git_fetch_options fetch_opts;
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
        fetch_opts.callbacks = create_remote_callbacks(&key_state);
        check_error(
            git_remote_fetch(remote.get(), &refspecs, &fetch_opts, nullptr),
            "Failed to fetch remote in " + path,
            repo.get());

        GitReference remote_ref;
        const int remote_lookup = git_reference_lookup(
            remote_ref.get_address(),
            repo.get(),
            tracking_ref.c_str());
        if (remote_lookup == GIT_ENOTFOUND) {
            throw std::runtime_error(
                "Missing remote branch " + remote_name + "/" + remote_branch +
                " in " + path);
        }
        check_error(
            remote_lookup,
            "Failed to lookup remote branch in " + path,
            repo.get());

        const git_oid *target_oid = git_reference_target(remote_ref.get());
        if (target_oid == nullptr) {
            throw std::runtime_error(
                "Missing target oid for remote branch " + remote_name + "/" +
                remote_branch + " in " + path);
        }

        GitReference new_branch_ref;
        if (local_exists) {
            check_error(
                git_reference_set_target(
                    new_branch_ref.get_address(),
                    local_ref.get(),
                    target_oid,
                    "Fast-forward"),
                "Failed to fast-forward branch in " + path,
                repo.get());
        } else {
            GitCommit target_commit;
            check_error(
                git_commit_lookup(
                    target_commit.get_address(),
                    repo.get(),
                    target_oid),
                "Failed to lookup target commit in " + path,
                repo.get());
            check_error(
                git_branch_create(
                    new_branch_ref.get_address(),
                    repo.get(),
                    local_branch.c_str(),
                    target_commit.get(),
                    0),
                "Failed to create branch in " + path,
                repo.get());
        }

        GitReference *upstream_ref =
            local_exists ? &local_ref : &new_branch_ref;
        const std::string upstream_name = remote_name + "/" + remote_branch;
        check_error(
            git_branch_set_upstream(upstream_ref->get(), upstream_name.c_str()),
            "Failed to set branch upstream in " + path,
            repo.get());

        if (is_current) {
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
                git_reset(
                    repo.get(),
                    target_obj.get(),
                    GIT_RESET_MIXED,
                    nullptr),
                "Failed to reset index in " + path,
                repo.get());

            git_checkout_options checkout_opts;
            git_checkout_options_init(
                &checkout_opts,
                GIT_CHECKOUT_OPTIONS_VERSION);
            checkout_opts.checkout_strategy =
                GIT_CHECKOUT_FORCE | GIT_CHECKOUT_REMOVE_UNTRACKED;
            check_error(
                git_checkout_head(repo.get(), &checkout_opts),
                "Failed to update working directory in " + path,
                repo.get());
        }

        if (stash_code != GIT_ENOTFOUND) {
            check_error(
                git_stash_pop(repo.get(), 0, nullptr),
                "Failed to pop stash in " + path,
                repo.get());
        }
    }

    void push_branch(
        const std::string &path,
        const std::string &remote_name,
        const std::string &local_branch,
        const std::string &remote_branch) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        GitReference local_ref;
        check_error(
            git_branch_lookup(
                local_ref.get_address(),
                repo.get(),
                local_branch.c_str(),
                GIT_BRANCH_LOCAL),
            "Failed to lookup branch in " + path,
            repo.get());

        GitRemote remote;
        check_error(
            git_remote_lookup(
                remote.get_address(),
                repo.get(),
                remote_name.c_str()),
            "Failed to lookup remote in " + path,
            repo.get());

        const std::string source_ref = "refs/heads/" + local_branch;
        const std::string target_ref = "refs/heads/" + remote_branch;
        std::string refspec = source_ref + ":" + target_ref;
        std::vector<char> refspec_buffer(refspec.begin(), refspec.end());
        refspec_buffer.push_back('\0');
        std::vector<char *> strings = {refspec_buffer.data()};
        git_strarray refspecs = {
            .strings = strings.data(),
            .count = strings.size()};

        SshKeyCursor key_state{
            .keys = cached_ssh_keys(),
            .next_index = 0,
        };
        git_push_options push_opts;
        git_push_options_init(&push_opts, GIT_PUSH_OPTIONS_VERSION);
        push_opts.callbacks = create_remote_callbacks(&key_state);
        check_error(
            git_remote_push(remote.get(), &refspecs, &push_opts),
            "Failed to push reference in " + path,
            repo.get());
    }

    BranchSyncState compare_branches(
        const std::string &path,
        const std::string &source_branch,
        const std::string &target_branch) override {
        GitRepository repo;
        check_error(
            git_repository_open(repo.get_address(), path.c_str()),
            "Failed to open repository in " + path,
            repo.get());

        const std::string source_ref = "refs/heads/" + source_branch;
        const std::string target_ref = "refs/heads/" + target_branch;
        const git_oid source_oid =
            lookup_ref_oid(repo.get(), source_ref.c_str());
        const git_oid target_oid =
            lookup_ref_oid(repo.get(), target_ref.c_str());

        size_t source_ahead_count = 0;
        size_t source_behind_count = 0;
        check_error(
            git_graph_ahead_behind(
                &source_ahead_count,
                &source_behind_count,
                repo.get(),
                &source_oid,
                &target_oid),
            "Failed to compare branches in " + path,
            repo.get());

        if (source_ahead_count == 0 && source_behind_count == 0) {
            return BranchSyncState::UP_TO_DATE;
        }
        if (source_ahead_count == 0 && source_behind_count > 0) {
            return BranchSyncState::TARGET_AHEAD;
        }
        if (source_ahead_count > 0 && source_behind_count > 0) {
            return BranchSyncState::DIVERGED;
        }
        return BranchSyncState::SOURCE_AHEAD;
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
