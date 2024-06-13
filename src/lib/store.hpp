#ifndef SRC_LIB_STORE_HPP_
#define SRC_LIB_STORE_HPP_

#include <vector>
#include <functional>
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
    virtual void addObserver(IObserver* observer) = 0;
    virtual void removeObserver(IObserver* observer) = 0;
    virtual void notifyObservers(EventType event) = 0;
};

class Store : public IObservable {
 public:
    static Store& getInstance() {
        static Store instance;
        return instance;
    }

    void addObserver(IObserver* observer) override {
        observers.push_back(observer);
    }

    void removeObserver(IObserver* observer) override {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }

    void notifyObservers(EventType event) override {
        for (auto observer : observers) {
            observer->update(event);
        }
    }

    void addTree(const Tree& tree) {
        trees.push_back(tree);
        notifyObservers(EventType::ADD_TREE);
    }

    const std::vector<Tree>& getTrees() const {
        return trees;
    }

 private:
    std::vector<Tree> trees;
    std::vector<IObserver*> observers;
    Store() = default;
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
};

#endif  // SRC_LIB_STORE_HPP_
