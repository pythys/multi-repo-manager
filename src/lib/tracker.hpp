#ifndef SRC_LIB_TRACKER_HPP_
#define SRC_LIB_TRACKER_HPP_

#include "tree.hpp"
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

enum class DisplayFormat : std::uint8_t;
class OutputView;

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

    void remove_repo(const std::string &root, const std::string &name);

  private:
    Repo *recursive_find(const std::string &name, std::vector<Repo> &repos);
    Repo &get_repo_locked(const std::string &root, const std::string &name);

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<Tree> trees_;
    std::deque<TrackerEvent> events_;
    bool closed_ = false;
};

class TrackedOperation {
  public:
    TrackedOperation(const std::vector<Tree> &trees, DisplayFormat format);
    ~TrackedOperation();

    Tracker &tracker();
    OutputView &view();

    TrackedOperation(const TrackedOperation &) = delete;
    TrackedOperation &operator=(const TrackedOperation &) = delete;

  private:
    Tracker tracker_;
    std::unique_ptr<OutputView> view_;
};

#endif // SRC_LIB_TRACKER_HPP_
