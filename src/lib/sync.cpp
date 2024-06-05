#include <tbb/parallel_for_each.h>
#include <filesystem>
#include <future>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include "config.hpp"
#include "sync.hpp"
#include "tree.hpp"
#include "repo_factory.hpp"

namespace fs = std::filesystem;
namespace asio = boost::asio;

enum class MatchType { TO_REMOVE, TO_ADD };
std::vector<Remote> find_remotes(
    const std::vector<Remote>& tree_remotes,
    const std::vector<Remote>& git_remotes,
    MatchType match_type) {

    std::vector<Remote> result;

    auto compare_by_name = [](const Remote& a, const Remote& b) {
        return a.name == b.name;
    };

    if (match_type == MatchType::TO_REMOVE) {
        for (const auto& git_remote : git_remotes) {
            auto it = std::find_if(
                tree_remotes.begin(),
                tree_remotes.end(),
                [&](const Remote& tree_remote) {
                    return compare_by_name(tree_remote, git_remote);
                });
            if (it == tree_remotes.end()) {
                result.push_back(git_remote);
            }
        }
    } else if (match_type == MatchType::TO_ADD) {
        for (const auto& tree_remote : tree_remotes) {
            auto it = std::find_if(
                git_remotes.begin(),
                git_remotes.end(),
                [&](const Remote& git_remote) {
                    return compare_by_name(git_remote, tree_remote);
                });
            if (it == git_remotes.end()) {
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
        }
        auto to_add = find_remotes(
            repo->remotes,
            remotes,
            MatchType::TO_ADD);
        for (const auto& remote : to_add) {
            repo_manager->add_remote(root + "/" + repo->name, remote);
        }
    };

    std::promise<void> clone_completed;
    std::future<void> clone_future = clone_completed.get_future();
    auto clone_action = [root, repo, &clone_completed]() {
        std::cout << "cloning repo:" + root + "/" + repo->name << std::endl;
        auto repo_manager = create_repo_manager(repo->type);
        repo_manager->copy(
            repo->remotes[0].url,
            root + "/" + repo->name);
        for (size_t i = 1; i < repo->remotes.size(); i++) {
            repo_manager->add_remote(
                root + "/" + repo->name,
                repo->remotes[i]);
        }
        clone_completed.set_value();
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
    asio::thread_pool pool(10);
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
