#include "output_view.hpp"
#include "tracker.hpp"
#include <gtest/gtest.h>
#include <sstream>

namespace {
std::vector<Tree> make_test_trees() {
    return {Tree{
        .root = "/test/root",
        .repos = {Repo{
            .name = "test-repo",
            .type = RepoType::GIT,
            .phase = RepoPhase::SUCCEEDED,
            .remotes = {Remote{
                .name = "origin",
                .url = "https://example.com/repo.git"}},
            .branches =
                {Branch{.name = "main", .remote = "origin", .is_current = true},
                 Branch{
                     .name = "develop",
                     .remote = "origin",
                     .is_current = false}},
            .children = {},
            .messages = {{.text = "Success", .level = MessageLevel::INFO}}}}}};
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
