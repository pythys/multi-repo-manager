#include "core/tracker.hpp"
#include "core/tree.hpp"
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <vector>

namespace {
constexpr int WAITER_SLEEP_MS = 10;

std::vector<Tree> make_trees() {
    return {Tree{
        .root = "root",
        .repos = {Repo{
            .name = "repo",
            .type = RepoType::GIT,
            .phase = RepoPhase::QUEUED,
            .remotes = {},
            .branches = {},
            .children = {},
            .messages = {}}}}};
}
} // namespace

TEST(TrackerTests, SetPhaseUpdatesSnapshotAndMessage) {
    Tracker tracker;
    tracker.populate(make_trees());
    tracker.set_phase("root", "repo", RepoPhase::RUNNING, "starting");

    const std::vector<Tree> snapshot = tracker.snapshot();
    ASSERT_EQ(1, snapshot.size());
    ASSERT_EQ(1, snapshot[0].repos.size());
    EXPECT_EQ(RepoPhase::RUNNING, snapshot[0].repos[0].phase);
    ASSERT_EQ(1, snapshot[0].repos[0].messages.size());
    EXPECT_EQ("starting", snapshot[0].repos[0].messages[0].text);
}

TEST(TrackerTests, WaitNextEventReturnsPublishedEvent) {
    Tracker tracker;
    tracker.populate(make_trees());
    tracker.set_phase("root", "repo", RepoPhase::SUCCEEDED, "done");

    TrackerEvent event;
    ASSERT_TRUE(tracker.wait_next_event(event));
    EXPECT_EQ("root", event.root);
    EXPECT_EQ("repo", event.repo);
    EXPECT_EQ(RepoPhase::SUCCEEDED, event.phase);
    EXPECT_EQ(MessageLevel::INFO, event.level);
    EXPECT_EQ("done", event.message);
}

TEST(TrackerTests, CloseUnblocksWaiter) {
    Tracker tracker;
    tracker.populate(make_trees());
    auto waiter = std::async(std::launch::async, [&tracker] {
        TrackerEvent event;
        return tracker.wait_next_event(event);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WAITER_SLEEP_MS));
    tracker.close();

    EXPECT_FALSE(waiter.get());
}

TEST(TrackerTests, EventOrderMatchesSetPhaseOrder) {
    Tracker tracker;
    tracker.populate(make_trees());
    tracker.set_phase("root", "repo", RepoPhase::RUNNING, "step1");
    tracker.set_phase("root", "repo", RepoPhase::SUCCEEDED, "step2");

    TrackerEvent first;
    TrackerEvent second;
    ASSERT_TRUE(tracker.wait_next_event(first));
    ASSERT_TRUE(tracker.wait_next_event(second));
    EXPECT_EQ("step1", first.message);
    EXPECT_EQ("step2", second.message);
}
