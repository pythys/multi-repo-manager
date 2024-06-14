#ifndef SRC_LIB_TRACKER_HPP_
#define SRC_LIB_TRACKER_HPP_

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include "tree.hpp"

class TreeObserver {
 public:
    virtual ~TreeObserver() = default;
    virtual void update() = 0;
};

class TreeObservable {
 public:
    virtual ~TreeObservable() = default;
    virtual void add_observer(TreeObserver* observer) = 0;
    virtual void remove_observer(TreeObserver* observer) = 0;
    virtual void notify_observers() = 0;
};

class Tracker : public TreeObservable {
 public:
    static Tracker& get_instance() {
        static Tracker instance;
        return instance;
    }

    void add_observer(TreeObserver* observer) override {
        observers.push_back(observer);
    }

    void remove_observer(TreeObserver* observer) override {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }

    void notify_observers() override {
        for (auto observer : observers) {
            observer->update();
        }
    }

    void populate(std::vector<Tree> initial) {
        this->trees = initial;
        notify_observers();
    }

    void set_status(
        const std::string& root,
        const std::string& name,
        RepoStatus status) {
        Repo& repo = get_repo(root, name);
        repo.status = status;
        notify_observers();
    }

    void add_message(
        const std::string& root,
        const std::string& name,
        const std::string& message) {
        Repo& repo = get_repo(root, name);
        repo.messages.push_back(message);
        notify_observers();
    }

    const std::vector<Tree>& get_trees() const {
        return trees;
    }

 private:
    std::vector<Tree> trees;
    std::vector<TreeObserver*> observers;

    // Singleton Pattern
    Tracker() = default;
    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;

    Repo* recursive_find(
        const std::string& name,
        const std::vector<Repo>& repos) {
        for (auto& repo : repos) {
            if (repo.name == name) {
                return const_cast<Repo*>(&repo);
            }
            Repo* found = recursive_find(name, repo.children);
            if (found) {
                return found;
            }
        }
        return nullptr;
    }

    Repo& get_repo(const std::string& root, const std::string& name) {
        for (auto& tree : trees) {
            if (tree.root == root) {
                Repo* found = recursive_find(name, tree.repos);
                if (found) {
                    return *found;
                }
            }
        }
        throw std::runtime_error("Repo not found");
    }
};

#endif  // SRC_LIB_TRACKER_HPP_
