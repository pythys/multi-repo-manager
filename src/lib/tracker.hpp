#ifndef SRC_LIB_TRACKER_HPP_
#define SRC_LIB_TRACKER_HPP_

/**
 * @file tracker.hpp
 * @brief Thread-safe tracker for repo phases/messages and streamed events.
 */

#include "tree.hpp"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

enum class MessageLevel : std::uint8_t { INFO, WARNING, ERROR };

/** Event emitted when repo state is updated. */
struct TrackerEvent {
    std::string root;
    std::string repo;
    RepoPhase phase = RepoPhase::QUEUED;
    MessageLevel level = MessageLevel::INFO;
    std::string message;
};

class Tracker {
  public:
    /** Replace tracked state with initial tree snapshot. */
    void populate(const std::vector<Tree> &initial);

    /**
     * @brief Update phase/message for a repo and emit one event.
     */
    void set_phase(
        const std::string &root,
        const std::string &repo,
        RepoPhase phase,
        const std::string &message = "",
        MessageLevel level = MessageLevel::INFO);

    /**
     * @brief Returns a copy of current tracked state.
     */
    std::vector<Tree> snapshot() const;

    /**
     * @brief Waits for next event.
     *
     * @return false when tracker is closed and no pending events remain.
     */
    bool wait_next_event(TrackerEvent &event);

    /** Closes tracker and unblocks waiting consumers. */
    void close();

  private:
    Repo *recursive_find(const std::string &name, std::vector<Repo> &repos);
    Repo &get_repo_locked(const std::string &root, const std::string &name);

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<Tree> trees_;
    std::deque<TrackerEvent> events_;
    bool closed_ = false;
};

#endif // SRC_LIB_TRACKER_HPP_
