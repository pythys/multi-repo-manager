#include <execution>
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

void sync_repository(
    const std::string& root,
    Repo* repo,
    asio::thread_pool* pool) {

    auto repo_manager = create_repo_manager(repo->type);
    std::promise<void> action_completed;
    std::future<void> action_future = action_completed.get_future();

    auto update_action = [root, repo, &repo_manager, &action_completed]() {
        std::cout << "updating repo: " + root + "/" + repo->name << std::endl;
        repo_manager->update(root + "/" + repo->name, repo->remotes[0].name);
        std::cout << "done updating: " + root + "/" + repo->name << std::endl;
        action_completed.set_value();
    };

    auto clone_action = [root, repo, &repo_manager, &action_completed]() {
        std::cout << "cloning repo: " + root + "/" + repo->name << std::endl;
        repo_manager->copy(repo->remotes[0].url, root + "/" + repo->name);
        std::cout << "done cloning: " + root + "/" + repo->name << std::endl;
        action_completed.set_value();
    };

    std::string repo_path = root + "/" + repo->name;
    fs::path repo_dir(repo_path);
    if (fs::exists(repo_dir) && fs::is_directory(repo_dir)) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
        action_future.wait();
    }
    for (auto& child : repo->children) {
        sync_repository(root, &child, pool);
    }
}

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    asio::thread_pool pool(10);
    std::for_each(
        std::execution::par,
        config.begin(),
        config.end(),
        [&pool](Tree& tree) {
            std::for_each(
                std::execution::par,
                tree.repos.begin(),
                tree.repos.end(),
                [&pool, &tree](Repo& repo) {
                    sync_repository(tree.root, &repo, &pool);
                });
        });
    pool.join();
    return 0;
}
