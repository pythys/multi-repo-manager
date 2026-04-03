#include "tracker.hpp"
#include "output_view.hpp"
#include "runtime.hpp"
#include <algorithm>
#include <stdexcept>

void Tracker::populate(const std::vector<Tree> &initial) {
    std::scoped_lock<std::mutex> lock(mutex_);
    trees_ = initial;
}

void Tracker::set_phase(
    const std::string &root,
    const std::string &repo,
    RepoPhase phase,
    const std::string &message,
    MessageLevel level) {
    std::scoped_lock<std::mutex> lock(mutex_);
    if (closed_) {
        return;
    }

    Repo &target = get_repo_locked(root, repo);
    target.phase = phase;
    if (!message.empty()) {
        target.messages.push_back({.text = message, .level = level});
    }
    events_.push_back(
        TrackerEvent{
            .root = root,
            .repo = repo,
            .phase = phase,
            .level = level,
            .message = message});
    condition_.notify_one();
}

std::vector<Tree> Tracker::snapshot() const {
    std::scoped_lock<std::mutex> lock(mutex_);
    return trees_;
}

bool Tracker::wait_next_event(TrackerEvent &event) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&] { return closed_ || !events_.empty(); });

    if (events_.empty()) {
        return false;
    }
    event = events_.front();
    events_.pop_front();
    return true;
}

void Tracker::close() {
    std::scoped_lock<std::mutex> lock(mutex_);
    closed_ = true;
    condition_.notify_all();
}

void Tracker::remove_repo(const std::string &root, const std::string &name) {
    std::scoped_lock<std::mutex> lock(mutex_);
    for (auto &tree : trees_) {
        if (tree.root != root) {
            continue;
        }
        auto result =
            std::ranges::remove_if(tree.repos, [&name](const Repo &repo) {
                return repo.name == name;
            });
        if (!result.empty()) {
            tree.repos.erase(result.begin(), result.end());
            return;
        }
    }
}

Repo *
Tracker::recursive_find(const std::string &name, std::vector<Repo> &repos) {
    for (auto &repo : repos) {
        if (repo.name == name) {
            return &repo;
        }
        Repo *found = recursive_find(name, repo.children);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}

Repo &
Tracker::get_repo_locked(const std::string &root, const std::string &name) {
    for (auto &tree : trees_) {
        if (tree.root != root) {
            continue;
        }
        Repo *found = recursive_find(name, tree.repos);
        if (found != nullptr) {
            return *found;
        }
    }
    throw std::runtime_error("Repo not found");
}

TrackedOperation::TrackedOperation(
    const std::vector<Tree> &trees,
    DisplayFormat format)
    : tracker_(), view_(nullptr) {
    tracker_.populate(trees);
    view_ = create_output_view(detect_output_mode(), format, tracker_);
    view_->start();
}

TrackedOperation::~TrackedOperation() {
    tracker_.close();
    view_->stop();
}

Tracker &TrackedOperation::tracker() {
    return tracker_;
}

OutputView &TrackedOperation::view() {
    return *view_;
}
