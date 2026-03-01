#ifndef TESTS_GIT_TEST_UTILS_HPP_
#define TESTS_GIT_TEST_UTILS_HPP_

#include <filesystem>
#include <git2.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace git_test {

template <typename T, void (*free_fn)(T *)> class GitHandle {
  public:
    explicit GitHandle(T *value = nullptr) : value_(value) {}
    ~GitHandle() {
        free_fn(value_);
    }
    GitHandle(const GitHandle &) = delete;
    GitHandle &operator=(const GitHandle &) = delete;
    GitHandle(GitHandle &&other) noexcept : value_(other.value_) {
        other.value_ = nullptr;
    }
    GitHandle &operator=(GitHandle &&other) noexcept {
        if (this != &other) {
            free_fn(value_);
            value_ = other.value_;
            other.value_ = nullptr;
        }
        return *this;
    }

    T *get() const {
        return value_;
    }
    T **out() {
        return &value_;
    }

  private:
    T *value_;
};

using GitRepository = GitHandle<git_repository, git_repository_free>;
using GitConfig = GitHandle<git_config, git_config_free>;
using GitIndex = GitHandle<git_index, git_index_free>;
using GitTree = GitHandle<git_tree, git_tree_free>;
using GitSignature = GitHandle<git_signature, git_signature_free>;
using GitCommit = GitHandle<git_commit, git_commit_free>;
using GitRemote = GitHandle<git_remote, git_remote_free>;
using GitReference = GitHandle<git_reference, git_reference_free>;
using GitAnnotatedCommit =
    GitHandle<git_annotated_commit, git_annotated_commit_free>;

inline std::string last_error() {
    const git_error *err = git_error_last();
    if (err == nullptr || err->message == nullptr) {
        return "unknown error";
    }
    return err->message;
}

inline void check(int code, const std::string &context) {
    if (code != 0) {
        throw std::runtime_error(context + ": " + last_error());
    }
}

inline GitRepository open_repo(const std::filesystem::path &path) {
    GitRepository repo;
    check(git_repository_open(repo.out(), path.c_str()), "open repo");
    return repo;
}

inline void init_repo(
    const std::filesystem::path &path,
    const std::string &branch,
    bool bare = false) {
    std::filesystem::create_directories(path.parent_path());
    git_repository_init_options opts;
    check(
        git_repository_init_options_init(
            &opts,
            GIT_REPOSITORY_INIT_OPTIONS_VERSION),
        "init options");
    opts.flags =
        GIT_REPOSITORY_INIT_MKPATH | (bare ? GIT_REPOSITORY_INIT_BARE : 0);
    opts.initial_head = branch.c_str();
    GitRepository repo;
    check(
        git_repository_init_ext(repo.out(), path.c_str(), &opts),
        "init repo");
}

inline void set_user(
    const std::filesystem::path &repo_path,
    const std::string &email,
    const std::string &name) {
    GitRepository repo = open_repo(repo_path);
    GitConfig config;
    check(git_repository_config(config.out(), repo.get()), "repo config");
    check(
        git_config_set_string(config.get(), "user.email", email.c_str()),
        "set email");
    check(
        git_config_set_string(config.get(), "user.name", name.c_str()),
        "set name");
}

inline void stage_all(const std::filesystem::path &repo_path) {
    GitRepository repo = open_repo(repo_path);
    GitIndex index;
    check(git_repository_index(index.out(), repo.get()), "repo index");
    check(
        git_index_add_all(
            index.get(),
            nullptr,
            GIT_INDEX_ADD_DEFAULT,
            nullptr,
            nullptr),
        "add all");
    check(git_index_write(index.get()), "index write");
}

inline void
stage_path(const std::filesystem::path &repo_path, const std::string &path) {
    GitRepository repo = open_repo(repo_path);
    GitIndex index;
    check(git_repository_index(index.out(), repo.get()), "repo index");
    check(git_index_add_bypath(index.get(), path.c_str()), "add path");
    check(git_index_write(index.get()), "index write");
}

inline void stage_rename(
    const std::filesystem::path &repo_path,
    const std::string &old_path,
    const std::string &new_path) {
    GitRepository repo = open_repo(repo_path);
    GitIndex index;
    check(git_repository_index(index.out(), repo.get()), "repo index");
    check(
        git_index_remove_bypath(index.get(), old_path.c_str()),
        "remove old path");
    check(git_index_add_bypath(index.get(), new_path.c_str()), "add new path");
    check(git_index_write(index.get()), "index write");
}

inline void
commit(const std::filesystem::path &repo_path, const std::string &message) {
    GitRepository repo = open_repo(repo_path);
    GitIndex index;
    check(git_repository_index(index.out(), repo.get()), "repo index");

    git_oid tree_oid;
    check(git_index_write_tree(&tree_oid, index.get()), "write tree");
    check(git_index_write(index.get()), "index write");

    GitTree tree;
    check(git_tree_lookup(tree.out(), repo.get(), &tree_oid), "lookup tree");

    GitSignature signature;
    check(
        git_signature_now(
            signature.out(),
            "mrm-tests",
            "mrm-tests@example.com"),
        "signature");

    git_oid parent_oid;
    const int parent_code =
        git_reference_name_to_id(&parent_oid, repo.get(), "HEAD");
    if (parent_code == 0) {
        GitCommit parent;
        check(
            git_commit_lookup(parent.out(), repo.get(), &parent_oid),
            "lookup parent");
        const git_commit *parents[] = {parent.get()};
        git_oid commit_oid;
        check(
            git_commit_create(
                &commit_oid,
                repo.get(),
                "HEAD",
                signature.get(),
                signature.get(),
                nullptr,
                message.c_str(),
                tree.get(),
                1,
                parents),
            "commit");
        return;
    }
    if (parent_code != GIT_ENOTFOUND && parent_code != GIT_EUNBORNBRANCH) {
        check(parent_code, "resolve HEAD");
    }

    git_oid commit_oid;
    check(
        git_commit_create(
            &commit_oid,
            repo.get(),
            "HEAD",
            signature.get(),
            signature.get(),
            nullptr,
            message.c_str(),
            tree.get(),
            0,
            nullptr),
        "initial commit");
}

inline void add_remote(
    const std::filesystem::path &repo_path,
    const std::string &name,
    const std::filesystem::path &url) {
    GitRepository repo = open_repo(repo_path);
    GitRemote remote;
    check(
        git_remote_create(remote.out(), repo.get(), name.c_str(), url.c_str()),
        "add remote");
}

inline void set_remote_url(
    const std::filesystem::path &repo_path,
    const std::string &name,
    const std::filesystem::path &url) {
    GitRepository repo = open_repo(repo_path);
    check(
        git_remote_set_url(repo.get(), name.c_str(), url.c_str()),
        "set remote url");
}

inline void push_refspec(
    const std::filesystem::path &repo_path,
    const std::string &remote_name,
    const std::string &refspec) {
    GitRepository repo = open_repo(repo_path);
    GitRemote remote;
    check(
        git_remote_lookup(remote.out(), repo.get(), remote_name.c_str()),
        "lookup remote");

    std::vector<char> refspec_buf(refspec.begin(), refspec.end());
    refspec_buf.push_back('\0');
    std::vector<char *> refs = {refspec_buf.data()};
    git_strarray refspecs = {.strings = refs.data(), .count = refs.size()};

    git_push_options push_opts;
    check(
        git_push_options_init(&push_opts, GIT_PUSH_OPTIONS_VERSION),
        "push opts");
    check(git_remote_push(remote.get(), &refspecs, &push_opts), "push");
}

inline void set_branch_upstream(
    const std::filesystem::path &repo_path,
    const std::string &branch,
    const std::string &upstream) {
    GitRepository repo = open_repo(repo_path);
    GitReference branch_ref;
    check(
        git_branch_lookup(
            branch_ref.out(),
            repo.get(),
            branch.c_str(),
            GIT_BRANCH_LOCAL),
        "lookup branch");
    check(
        git_branch_set_upstream(branch_ref.get(), upstream.c_str()),
        "set upstream");
}

inline void push_branch(
    const std::filesystem::path &repo_path,
    const std::string &remote_name,
    const std::string &branch,
    bool set_upstream) {
    const std::string local_ref = "refs/heads/" + branch;
    const std::string remote_ref = "refs/heads/" + branch;
    push_refspec(repo_path, remote_name, local_ref + ":" + remote_ref);
    if (set_upstream) {
        set_branch_upstream(repo_path, branch, remote_name + "/" + branch);
    }
}

inline void delete_remote_branch(
    const std::filesystem::path &repo_path,
    const std::string &remote_name,
    const std::string &branch) {
    push_refspec(repo_path, remote_name, ":refs/heads/" + branch);
}

inline void checkout_branch(
    const std::filesystem::path &repo_path,
    const std::string &branch) {
    GitRepository repo = open_repo(repo_path);
    const std::string ref_name = "refs/heads/" + branch;
    check(git_repository_set_head(repo.get(), ref_name.c_str()), "set head");
    git_checkout_options opts;
    check(
        git_checkout_options_init(&opts, GIT_CHECKOUT_OPTIONS_VERSION),
        "checkout opts");
    opts.checkout_strategy = GIT_CHECKOUT_SAFE | GIT_CHECKOUT_RECREATE_MISSING;
    check(git_checkout_head(repo.get(), &opts), "checkout");
}

inline void create_and_checkout_branch(
    const std::filesystem::path &repo_path,
    const std::string &branch) {
    GitRepository repo = open_repo(repo_path);
    git_oid head_oid;
    check(git_reference_name_to_id(&head_oid, repo.get(), "HEAD"), "head oid");
    GitCommit head_commit;
    check(
        git_commit_lookup(head_commit.out(), repo.get(), &head_oid),
        "head commit");
    GitReference branch_ref;
    check(
        git_branch_create(
            branch_ref.out(),
            repo.get(),
            branch.c_str(),
            head_commit.get(),
            0),
        "create branch");
    checkout_branch(repo_path, branch);
}

inline bool merge_branch(
    const std::filesystem::path &repo_path,
    const std::string &branch) {
    GitRepository repo = open_repo(repo_path);
    GitReference branch_ref;
    check(
        git_branch_lookup(
            branch_ref.out(),
            repo.get(),
            branch.c_str(),
            GIT_BRANCH_LOCAL),
        "lookup merge branch");
    GitAnnotatedCommit annotated;
    check(
        git_annotated_commit_from_ref(
            annotated.out(),
            repo.get(),
            branch_ref.get()),
        "annotated commit");
    const git_annotated_commit *annotated_ptr = annotated.get();
    git_merge_options merge_opts;
    check(
        git_merge_options_init(&merge_opts, GIT_MERGE_OPTIONS_VERSION),
        "merge opts");
    git_checkout_options checkout_opts;
    check(
        git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION),
        "merge checkout opts");
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
    const int merge_code =
        git_merge(repo.get(), &annotated_ptr, 1, &merge_opts, &checkout_opts);
    if (merge_code != 0 && merge_code != GIT_EMERGECONFLICT) {
        check(merge_code, "merge");
    }
    GitIndex index;
    check(git_repository_index(index.out(), repo.get()), "repo index");
    return git_index_has_conflicts(index.get()) == 0;
}

inline void fetch_remote(
    const std::filesystem::path &repo_path,
    const std::string &remote_name) {
    GitRepository repo = open_repo(repo_path);
    GitRemote remote;
    check(
        git_remote_lookup(remote.out(), repo.get(), remote_name.c_str()),
        "lookup remote");
    git_fetch_options fetch_opts;
    check(
        git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION),
        "fetch opts");
    check(
        git_remote_fetch(remote.get(), nullptr, &fetch_opts, nullptr),
        "fetch");
}

inline bool ref_exists(
    const std::filesystem::path &repo_path,
    const std::string &ref_name) {
    GitRepository repo = open_repo(repo_path);
    GitReference ref;
    return git_reference_lookup(ref.out(), repo.get(), ref_name.c_str()) == 0;
}

inline std::pair<int, int> left_right_counts(
    const std::filesystem::path &repo_path,
    const std::string &left_ref,
    const std::string &right_ref) {
    GitRepository repo = open_repo(repo_path);
    GitReference left;
    check(
        git_reference_lookup(left.out(), repo.get(), left_ref.c_str()),
        "lookup left ref");
    GitReference right;
    check(
        git_reference_lookup(right.out(), repo.get(), right_ref.c_str()),
        "lookup right ref");
    const git_oid *left_oid = git_reference_target(left.get());
    const git_oid *right_oid = git_reference_target(right.get());
    if (left_oid == nullptr || right_oid == nullptr) {
        throw std::runtime_error(
            "symbolic refs not supported in left_right_counts");
    }
    size_t right_ahead = 0;
    size_t right_behind = 0;
    check(
        git_graph_ahead_behind(
            &right_ahead,
            &right_behind,
            repo.get(),
            right_oid,
            left_oid),
        "ahead/behind");
    return {static_cast<int>(right_behind), static_cast<int>(right_ahead)};
}

} // namespace git_test

#endif // TESTS_GIT_TEST_UTILS_HPP_
