#include "sync.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"
#include <algorithm>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <tbb/parallel_for_each.h>
#include <vector>

namespace asio = boost::asio;

enum class MatchType : std::uint8_t { TO_REMOVE, TO_ADD };
std::vector<Remote> find_remotes(
    const std::vector<Remote> &conf_remotes,
    const std::vector<Remote> &repo_remotes,
    MatchType match_type) {

    std::vector<Remote> result;

    auto compare_by_name = [](const Remote &a, const Remote &b) {
        return a.name == b.name;
    };

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto &repo_remote : repo_remotes) {
            auto it = std::ranges::find_if(
                conf_remotes,
                [&](const Remote &tree_remote) {
                    return compare_by_name(tree_remote, repo_remote);
                });
            if (it == conf_remotes.end()) {
                result.push_back(repo_remote);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto &tree_remote : conf_remotes) {
            auto it = std::ranges::find_if(
                repo_remotes,
                [&](const Remote &repo_remote) {
                    return compare_by_name(repo_remote, tree_remote);
                });
            if (it == repo_remotes.end()) {
                result.push_back(tree_remote);
            }
        }
    }
    return result;
}

void update_repository(const std::string &root, Repo *repo) {
    std::cout << "updating repo: " + root + "/" + repo->name << '\n';
    auto repo_manager = create_repo_manager(repo->type);
    auto remotes = repo_manager->get_remotes(root + "/" + repo->name);
    auto to_remove = find_remotes(repo->remotes, remotes, MatchType::TO_REMOVE);
    for (const auto &remote : to_remove) {
        repo_manager->remove_remote(root + "/" + repo->name, remote);
    }
    auto to_add = find_remotes(repo->remotes, remotes, MatchType::TO_ADD);
    for (const auto &remote : to_add) {
        repo_manager->add_remote(root + "/" + repo->name, remote);
    }
}

void clone_repository(const std::string &root, Repo *repo) {
    std::cout << "cloning repo:" + root + "/" + repo->name << '\n';
    auto repo_manager = create_repo_manager(repo->type);
    auto it = std::ranges::find_if(repo->remotes, [](const Remote &remote) {
        return remote.name == "origin";
    });
    if (it != repo->remotes.end()) {
        repo_manager->copy(it->url, root + "/" + repo->name);
    } else {
        std::cerr << "No remote found with name 'origin' for repo: "
                  << repo->name << '\n';
        std::exit(1);
    }
    for (size_t i = 0; i < repo->remotes.size(); i++) {
        if (repo->remotes[i].name == "origin") {
            continue;
        }
        repo_manager->add_remote(root + "/" + repo->name, repo->remotes[i]);
    }
}

void sync_repository(
    const std::string &root,
    Repo *repo,
    asio::thread_pool *pool) {

    auto update_action = [root, repo, pool]() {
        try {
            update_repository(root, repo);
        } catch (const std::exception &e) {
            std::cerr << "Error updating repository " << repo->name << ": "
                      << e.what() << '\n';
            return;
        }
        for (auto &child : repo->children) {
            sync_repository(root, &child, pool);
        }
    };

    auto clone_action = [root, repo, pool]() {
        try {
            clone_repository(root, repo);
        } catch (const std::exception &e) {
            std::cerr << "Error cloning repository " << repo->name << ": "
                      << e.what() << '\n';
            return;
        }
        for (auto &child : repo->children) {
            sync_repository(root, &child, pool);
        }
    };

    auto repo_manager = create_repo_manager(repo->type);
    auto is_repo = repo_manager->is_repo(root + "/" + repo->name);
    if (is_repo) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
    }
}

int run_sync(const std::string &config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    asio::thread_pool pool(SYNC_POOL_SIZE);
    for (auto &tree : config) {
        for (auto &repo : tree.repos) {
            sync_repository(tree.root, &repo, &pool);
        }
    }
    pool.join();
    return 0;
}
