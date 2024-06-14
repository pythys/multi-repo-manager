#include <tbb/parallel_for_each.h>
#include <filesystem>
#include <future>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include "config.hpp"
#include "constants.hpp"
#include "repo_factory.hpp"
#include "sync.hpp"
#include "tracker.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;
namespace asio = boost::asio;

enum class MatchType { TO_REMOVE, TO_ADD };
std::vector<Remote> find_remotes(
    const std::vector<Remote>& tree_remotes,
    const std::vector<Remote>& repo_remotes,
    MatchType match_type) {

    std::vector<Remote> result;

    auto compare_by_name = [](const Remote& a, const Remote& b) {
        return a.name == b.name;
    };

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto& repo_remote : repo_remotes) {
            auto it = std::find_if(
                tree_remotes.begin(),
                tree_remotes.end(),
                [&](const Remote& tree_remote) {
                    return compare_by_name(tree_remote, repo_remote);
                });
            if (it == tree_remotes.end()) {
                result.push_back(repo_remote);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto& tree_remote : tree_remotes) {
            auto it = std::find_if(
                repo_remotes.begin(),
                repo_remotes.end(),
                [&](const Remote& repo_remote) {
                    return compare_by_name(repo_remote, tree_remote);
                });
            if (it == repo_remotes.end()) {
                result.push_back(tree_remote);
            }
        }
    }
    return result;
}

void sync_repository(
    const std::string& root,
    Repo* repo,
    asio::thread_pool* pool) {

    Tracker::get_instance().set_status(root, repo->name, RepoStatus::SYNCHING);
    auto update_action = [root, repo]() {
        std::cout << "updating repo: " + root + "/" + repo->name << std::endl;
        auto repo_manager = create_repo_manager(repo->type);
        auto remotes = repo_manager->get_remotes(root + "/" + repo->name);
        auto to_remove = find_remotes(
            repo->remotes,
            remotes,
            MatchType::TO_REMOVE);
        for (const auto& remote : to_remove) {
            repo_manager->remove_remote(root + "/" + repo->name, remote);
            std::string message = "Removed from repo: "
                + root
                + "/"
                + repo->name
                + " remote: "
                + remote.name;
            Tracker::get_instance().add_message(root, repo->name, message);
        }
        auto to_add = find_remotes(
            repo->remotes,
            remotes,
            MatchType::TO_ADD);
        for (const auto& remote : to_add) {
            repo_manager->add_remote(root + "/" + repo->name, remote);
            std::string message = "Added to repo: "
                + root
                + "/"
                + repo->name
                + " remote: "
                + remote.name;
            Tracker::get_instance().add_message(root, repo->name, message);
        }
        Tracker::get_instance().set_status(
            root,
            repo->name,
            RepoStatus::SYNCHED);
    };

    std::promise<void> clone_completed;
    std::future<void> clone_future = clone_completed.get_future();
    auto clone_action = [root, repo, &clone_completed]() {
        std::cout << "cloning repo:" + root + "/" + repo->name << std::endl;
        auto repo_manager = create_repo_manager(repo->type);
        auto it = std::find_if(
            repo->remotes.begin(),
            repo->remotes.end(),
            [](const Remote& remote) {
                return remote.name == "origin";
            });
        if (it != repo->remotes.end()) {
            repo_manager->copy(it->url, root + "/" + repo->name);
        } else {
            std::cerr << "No remote found with name 'origin' for repo: "
                      << repo->name
                      << std::endl;
            std::exit(1);
        }
        for (size_t i = 0; i < repo->remotes.size(); i++) {
            repo_manager->add_remote(
                root + "/" + repo->name,
                repo->remotes[i]);
        }
        clone_completed.set_value();
        Tracker::get_instance().set_status(
            root,
            repo->name,
            RepoStatus::SYNCHED);
    };

    fs::path repo_dir(root + "/" + repo->name);
    if (fs::exists(repo_dir) && fs::is_directory(repo_dir)) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
        clone_future.wait();
    }
    for (auto& child : repo->children) {
        sync_repository(root, &child, pool);
    }
}

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    Tracker::get_instance().populate(config);
    asio::thread_pool pool(SYNC_POOL_SIZE);
    tbb::parallel_for_each(
        config.begin(),
        config.end(),
        [&pool](Tree& tree) {
            tbb::parallel_for_each(
                tree.repos.begin(),
                tree.repos.end(),
                [&pool, &tree](Repo& repo) {
                    sync_repository(tree.root, &repo, &pool);
                });
        });
    pool.join();
    return 0;
}
