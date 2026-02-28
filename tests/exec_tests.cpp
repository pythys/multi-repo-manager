#include "exec.hpp"
#include "git_guard.hpp"
#include <chrono>
#include <cstdlib>
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
        path_ = fs::temp_directory_path() / ("mrm-exec-tests-" + unique);
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

int run_git(const fs::path &repo, const std::string &command) {
    const std::string cmd =
        "git -C \"" + repo.string() + "\" " + command + " >/dev/null 2>&1";
    return std::system(cmd.c_str());
}

void write_exec_config(
    const fs::path &config_path,
    const fs::path &root,
    const std::string &repo_name) {
    std::ofstream out(config_path);
    out << "trees:\n";
    out << "- root: " << root.string() << "\n";
    out << "  repos:\n";
    out << "  - name: " << repo_name << "\n";
    out << "    type: git\n";
    out << "    remotes:\n";
    out << "    - name: origin\n";
    out << "      url: " << (root / "remote.git").string() << "\n";
    out << "    branches:\n";
    out << "    - name: main\n";
    out << "      remote: origin\n";
    out << "      is_current: true\n";
}
} // namespace

TEST(ExecTests, WrapsGitSubcommandsWhenGitCliExists) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));

    const fs::path config = temp.path() / "config.yml";
    write_exec_config(config, root, "repo1");

    testing::internal::CaptureStdout();
    const int code =
        run_exec("rev-parse --is-inside-work-tree", config.string(), "git");
    const std::string stdout_text = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, code);
    EXPECT_NE(std::string::npos, stdout_text.find("true"));
}

TEST(ExecTests, AvoidsDoubleWrapWhenCommandAlreadyHasCli) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));

    const fs::path config = temp.path() / "config.yml";
    write_exec_config(config, root, "repo1");

    testing::internal::CaptureStdout();
    const int code =
        run_exec("git rev-parse --is-inside-work-tree", config.string(), "git");
    const std::string stdout_text = testing::internal::GetCapturedStdout();

    EXPECT_EQ(0, code);
    EXPECT_NE(std::string::npos, stdout_text.find("true"));
}

TEST(ExecTests, InvalidRepoTypeReturnsError) {
    TempDir temp;
    const fs::path config = temp.path() / "config.yml";
    write_exec_config(config, temp.path(), "repo1");
    EXPECT_EQ(1, run_exec("status", config.string(), "invalid"));
}

TEST(ExecTests, RepoTypeFilterSkipsMismatchedRepos) {
    TempDir temp;
    const fs::path root = temp.path() / "root";
    const fs::path repo = root / "repo1";
    fs::create_directories(repo);
    ASSERT_EQ(0, run_git(repo, "init -b main"));
    ASSERT_EQ(0, run_git(repo, "config user.email test@example.com"));
    ASSERT_EQ(0, run_git(repo, "config user.name test"));

    const fs::path config = temp.path() / "config.yml";
    write_exec_config(config, root, "repo1");

    EXPECT_EQ(
        0,
        run_exec("this-command-should-not-run", config.string(), "svn"));
}
