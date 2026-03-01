#include "git_guard.hpp"
#include "git_test_utils.hpp"
#include "remotesync.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace fs = std::filesystem;

namespace {
const GitGuard git_guard;

class TempDir {
  public:
    TempDir() {
        const auto unique = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = fs::temp_directory_path() / ("mrm-remotesync-tests-" + unique);
        fs::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }

    const fs::path &path() const {
        return path_;
    }

  private:
    fs::path path_;
};

void write_config(const fs::path &config, const fs::path &root) {
    std::ofstream out(config);
    out << "trees:\n";
    out << "- root: " << root.string() << "\n";
    out << "  repos:\n";
    out << "  - name: repo1\n";
    out << "    type: git\n";
    out << "    remotes:\n";
    out << "    - name: origin\n";
    out << "      url: placeholder\n";
    out << "    - name: upstream\n";
    out << "      url: placeholder\n";
    out << "    branches:\n";
    out << "    - name: main\n";
    out << "      remote: origin\n";
    out << "      is_current: true\n";
}

void setup_repo_with_remotes(
    const fs::path &temp_path,
    fs::path &repo,
    fs::path &origin,
    fs::path &upstream) {
    repo = temp_path / "root" / "repo1";
    origin = temp_path / "origin.git";
    upstream = temp_path / "upstream.git";
    fs::create_directories(repo);
    git_test::init_repo(origin, "main", true);
    git_test::init_repo(upstream, "main", true);
    git_test::init_repo(repo, "main");
    git_test::set_user(repo, "test@example.com", "test");
    std::ofstream readme(repo / "README.md");
    readme << "initial\n";
    readme.close();
    git_test::stage_all(repo);
    git_test::commit(repo, "init");
    git_test::add_remote(repo, "origin", origin);
    git_test::add_remote(repo, "upstream", upstream);
    git_test::push_branch(repo, "origin", "main", true);
    git_test::push_branch(repo, "upstream", "main", true);
}
} // namespace

TEST(RemoteSyncTests, DryRunReportsPlannedPush) {
    TempDir temp;
    fs::path repo;
    fs::path origin;
    fs::path upstream;
    setup_repo_with_remotes(temp.path(), repo, origin, upstream);

    std::ofstream change(repo / "README.md", std::ios::app);
    change << "upstream change\n";
    change.close();
    git_test::stage_all(repo);
    git_test::commit(repo, "upstream-change");
    git_test::push_branch(repo, "upstream", "main", false);

    const fs::path config = temp.path() / "config.yml";
    write_config(config, temp.path() / "root");

    testing::internal::CaptureStdout();
    const int code =
        run_remotesync(config.string(), "upstream", "origin", {"main"}, true);
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, code);
    EXPECT_NE(std::string::npos, output.find("DRY-RUN - Would push"));
}

TEST(RemoteSyncTests, MissingTargetBranchIsSkipped) {
    TempDir temp;
    fs::path repo;
    fs::path origin;
    fs::path upstream;
    setup_repo_with_remotes(temp.path(), repo, origin, upstream);
    git_test::delete_remote_branch(repo, "origin", "main");

    const fs::path config = temp.path() / "config.yml";
    write_config(config, temp.path() / "root");

    testing::internal::CaptureStdout();
    const int code =
        run_remotesync(config.string(), "upstream", "origin", {"main"}, false);
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, code);
    EXPECT_NE(
        std::string::npos,
        output.find("SKIPPED - Missing target branch"));
    EXPECT_FALSE(git_test::ref_exists(origin, "refs/heads/main"));
}

TEST(RemoteSyncTests, FallsBackToLocalWhenSourceFetchFails) {
    TempDir temp;
    fs::path repo;
    fs::path origin;
    fs::path upstream;
    setup_repo_with_remotes(temp.path(), repo, origin, upstream);

    std::ofstream change(repo / "README.md", std::ios::app);
    change << "local fallback change\n";
    change.close();
    git_test::stage_all(repo);
    git_test::commit(repo, "local-change");
    git_test::set_remote_url(
        repo,
        "upstream",
        temp.path() / "missing-upstream.git");

    const fs::path config = temp.path() / "config.yml";
    write_config(config, temp.path() / "root");

    testing::internal::CaptureStdout();
    const int code =
        run_remotesync(config.string(), "upstream", "origin", {"main"}, false);
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, code);
    EXPECT_NE(std::string::npos, output.find("SYNCED"));

    git_test::fetch_remote(repo, "origin");
    const auto [left, right] = git_test::left_right_counts(
        repo,
        "refs/remotes/origin/main",
        "refs/heads/main");
    EXPECT_EQ(0, left);
    EXPECT_EQ(0, right);
}
