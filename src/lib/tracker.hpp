#ifndef SRC_LIB_TRACKER_HPP_
#define SRC_LIB_TRACKER_HPP_

#include "tree.hpp"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

struct TrackerEvent {
    std::string root;
    std::string repo;
    RepoPhase phase = RepoPhase::QUEUED;
    MessageLevel level = MessageLevel::INFO;
    std::string message;
};

class Tracker {
  public:
    void populate(const std::vector<Tree> &initial);

    void set_phase(
        const std::string &root,
        const std::string &repo,
        RepoPhase phase,
        const std::string &message = "",
        MessageLevel level = MessageLevel::INFO);

    std::vector<Tree> snapshot() const;

    bool wait_next_event(TrackerEvent &event);

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
