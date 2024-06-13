#ifndef SRC_LIB_STORE_HPP_
#define SRC_LIB_STORE_HPP_

#include <vector>
#include <functional>
#include "tree.hpp"

class IObserver {
 public:
    virtual ~IObserver() = default;
    virtual void update() = 0;
};

class IObservable {
 public:
    virtual ~IObservable() = default;
    virtual void addObserver(IObserver* observer) = 0;
    virtual void removeObserver(IObserver* observer) = 0;
    virtual void notifyObservers() = 0;
};

class Store : public IObservable {
 public:
    void addObserver(IObserver* observer) override {
        observers.push_back(observer);
    }

    void removeObserver(IObserver* observer) override {
        observers.erase(
            std::remove(observers.begin(), observers.end(), observer),
            observers.end());
    }

    void notifyObservers() override {
        for (auto observer : observers) {
            observer->update();
        }
    }

    void addTree(const Tree& tree) {
        trees.push_back(tree);
        notifyObservers();
    }

    const std::vector<Tree>& getTrees() const {
        return trees;
    }

 private:
    std::vector<Tree> trees;
    std::vector<IObserver*> observers;
};

#endif  // SRC_LIB_STORE_HPP_
