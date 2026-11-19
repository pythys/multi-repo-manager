#include "core/tracker.hpp"
#include "presentation/output_view.hpp"
#include <gtest/gtest.h>
#include <string>

namespace {
std::vector<Tree> make_test_trees() {
    return {
        Tree{
            .root = "/test/root",
            .repos =
                {
                    Repo{
                        .name = "test-repo",
                        .phase = RepoPhase::SUCCEEDED,
                        .remotes =
                            {
                                Remote{
                                    .name = "origin",
                                    .url = "https://example.com/repo.git",
                                },
                            },
                        .branches =
                            {
                                Branch{
                                    .name = "main",
                                    .remote = "origin",
                                    .is_current = true,
                                },
                                Branch{
                                    .name = "develop",
                                    .remote = "origin",
                                    .is_current = false,
                                },
                            },
                        .children = {},
                        .messages =
                            {
                                {
                                    .text = "Success",
                                    .level = MessageLevel::INFO,
                                },
                            },
                    },
                },
        },
    };
}

std::vector<Tree> make_quiet_test_trees() {
    return {
        Tree{
            .root = "/test/root",
            .repos =
                {
                    Repo{
                        .name = "test-repo",
                        .phase = RepoPhase::SUCCEEDED,
                        .remotes = {},
                        .branches = {},
                        .children = {},
                        .messages =
                            {
                                {
                                    .text = "info message",
                                    .level = MessageLevel::INFO,
                                },
                                {
                                    .text = "output line",
                                    .level = MessageLevel::OUTPUT,
                                },
                                {
                                    .text = "warning line",
                                    .level = MessageLevel::WARNING,
                                },
                                {
                                    .text = "error line",
                                    .level = MessageLevel::ERROR,
                                },
                            },
                    },
                },
        },
    };
}
} // namespace

TEST(OutputViewTests, CreateOutputViewProgressModeTUI) {
    Tracker tracker;
    auto view =
        create_output_view(OutputMode::TUI, DisplayFormat::PROGRESS, tracker);
    EXPECT_NE(nullptr, view);
}

TEST(OutputViewTests, CreateOutputViewProgressModeTEXT) {
    Tracker tracker;
    auto view =
        create_output_view(OutputMode::TEXT, DisplayFormat::PROGRESS, tracker);
    EXPECT_NE(nullptr, view);
}

TEST(OutputViewTests, CreateOutputViewTableModeTUI) {
    Tracker tracker;
    auto view =
        create_output_view(OutputMode::TUI, DisplayFormat::TABLE, tracker);
    EXPECT_NE(nullptr, view);
}

TEST(OutputViewTests, CreateOutputViewTableModeTEXT) {
    Tracker tracker;
    auto view =
        create_output_view(OutputMode::TEXT, DisplayFormat::TABLE, tracker);
    EXPECT_NE(nullptr, view);
}

TEST(OutputViewTests, TextViewTableFormatCanStart) {
    Tracker tracker;
    tracker.populate(make_test_trees());
    tracker.close();

    auto view =
        create_output_view(OutputMode::TEXT, DisplayFormat::TABLE, tracker);
    view->start();
    view->stop();
}

TEST(OutputViewTests, TextViewProgressFormatCanStart) {
    Tracker tracker;
    tracker.populate(make_test_trees());
    tracker.close();

    auto view =
        create_output_view(OutputMode::TEXT, DisplayFormat::PROGRESS, tracker);
    view->start();
    view->stop();
}

TEST(OutputViewTests, QuietProgressPrintsOnlyOutputAndDiagnostics) {
    Tracker tracker;
    tracker.populate(make_quiet_test_trees());
    tracker.close();

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto view = create_output_view(
        OutputMode::TEXT,
        DisplayFormat::PROGRESS,
        tracker,
        OutputStyle::QUIET);
    view->start();
    view->stop();
    view.reset();

    const std::string output = testing::internal::GetCapturedStdout();
    const std::string diagnostics = testing::internal::GetCapturedStderr();

    EXPECT_EQ("output line\n", output);
    EXPECT_EQ(
        "/test/root/test-repo: warning line\n"
        "/test/root/test-repo: error line\n",
        diagnostics);
    EXPECT_EQ(std::string::npos, output.find("mrm report"));
    EXPECT_EQ(std::string::npos, output.find("SUCCEEDED"));
    EXPECT_EQ(std::string::npos, output.find("info message"));
}

TEST(OutputViewTests, QuietProgressForcesPlainTextInTuiMode) {
    Tracker tracker;
    tracker.populate(make_quiet_test_trees());
    tracker.close();

    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    auto view = create_output_view(
        OutputMode::TUI,
        DisplayFormat::PROGRESS,
        tracker,
        OutputStyle::QUIET);
    view->start();
    view->stop();
    view.reset();

    const std::string output = testing::internal::GetCapturedStdout();
    const std::string diagnostics = testing::internal::GetCapturedStderr();

    EXPECT_EQ("output line\n", output);
    EXPECT_EQ(
        "/test/root/test-repo: warning line\n"
        "/test/root/test-repo: error line\n",
        diagnostics);
}

TEST(OutputViewTests, QuietTableOmitsTitleAndKeepsRows) {
    Tracker tracker;
    tracker.populate(make_test_trees());
    tracker.close();

    testing::internal::CaptureStdout();
    auto view = create_output_view(
        OutputMode::TEXT,
        DisplayFormat::TABLE,
        tracker,
        OutputStyle::QUIET);
    view->start();
    view->stop();
    view.reset();

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(std::string::npos, output.find("mrm list"));
    EXPECT_NE(std::string::npos, output.find("ROOT"));
    EXPECT_NE(std::string::npos, output.find("test-repo"));
}

TEST(OutputViewTests, QuietSummaryOmitsTitleAndKeepsTotals) {
    Tracker tracker;
    tracker.populate(make_test_trees());
    tracker.close();

    testing::internal::CaptureStdout();
    auto view = create_output_view(
        OutputMode::TEXT,
        DisplayFormat::SUMMARY,
        tracker,
        OutputStyle::QUIET);
    view->start();
    view->stop();
    view.reset();

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(std::string::npos, output.find("mrm list"));
    EXPECT_NE(std::string::npos, output.find("/test/root"));
    EXPECT_NE(std::string::npos, output.find("TOTAL"));
}
