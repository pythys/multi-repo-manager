#ifndef SRC_LIB_TRACKER_HPP_
#define SRC_LIB_TRACKER_HPP_

#include <functional>
#include <vector>
#include "tree.hpp"

enum class EventType {
    ADD_TREE
};

class IObserver {
 public:
    virtual ~IObserver() = default;
    virtual void update(EventType event) = 0;
};

class IObservable {
 public:
    virtual ~IObservable() = default;
    virtual void add_observer(IObserver* observer) = 0;
    virtual void remove_observer(IObserver* observer) = 0;
    virtual void notify_observers(EventType event) = 0;
};

class Tracker : public IObservable {
 public:
    static Tracker& get_instance() {
        static Tracker instance;
        return instance;
    }

    void add_observer(IObserver* observer) override {
        observers.push_back(observer);
    }

    void remove_observer(IObserver* observer) override {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }

    void notify_observers(EventType event) override {
        for (auto observer : observers) {
            observer->update(event);
        }
    }

    void add_tree(const Tree& tree) {
        trees.push_back(tree);
        notify_observers(EventType::ADD_TREE);
    }

    const std::vector<Tree>& get_trees() const {
        return trees;
    }

 private:
    std::vector<Tree> trees;
    std::vector<IObserver*> observers;
    Tracker() = default;
    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;
};

#endif  // SRC_LIB_TRACKER_HPP_
