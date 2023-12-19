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
    const Repo& repo,
    asio::thread_pool* pool) {

    auto update_action = [root, repo]() {
        std::cout << "updating repo: " + root + "/" + repo.name << std::endl;
    };

    std::promise<void> clone_completed;
    std::future<void> clone_future = clone_completed.get_future();
    auto clone_action = [root, repo, &clone_completed]() {
        std::cout << "cloning repo:" + root + "/" + repo.name << std::endl;
        auto repo_manager = createRepoManager(repo.type);
        repo_manager->copy(
            repo.remotes[0].url,
            root + "/" + repo.name);
        clone_completed.set_value();
    };

    std::string repo_path = root + "/" + repo.name;
    fs::path repo_dir(repo_path);
    if (fs::exists(repo_dir) && fs::is_directory(repo_dir)) {
        asio::post(*pool, update_action);
    } else {
        asio::post(*pool, clone_action);
        clone_future.wait();
    }
    for (auto& child : repo.children) {
        sync_repository(root, child, pool);
    }
}

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    asio::thread_pool pool(10);
    for (const auto& tree : config) {
        for (const auto& repo : tree.repos) {
            sync_repository(tree.root, repo, &pool);
        }
    }
    pool.join();
    return 0;
}
