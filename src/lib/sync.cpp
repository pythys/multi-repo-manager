#include <boost/asio.hpp>
#include <filesystem>
#include <iostream>
#include <vector>
#include "config.hpp"
#include "sync.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;
namespace asio = boost::asio;

void sync_repository(
    const std::string& root,
    const Repo& repo,
    asio::io_context& io_context,
    asio::thread_pool& pool) {

    auto sync_action = [root, repo, &io_context, &pool]() {
        std::string repo_path = root + "/" + repo.name;
        fs::path repo_dir(repo_path);
        if (fs::exists(repo_dir) && fs::is_directory(repo_dir)) {
            std::cout << "updating repo: " + repo_dir.string() << std::endl;
        } else {
            std::cout << "cloning repo:" + repo_dir.string() << std::endl;
        }
        for (auto& child : repo.children) {
            sync_repository(repo_path, child, io_context, pool);
        }
    };
    asio::post(pool, sync_action);
}

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    asio::io_context io_context;
    asio::thread_pool pool(10);
    for (const auto& tree : config) {
        for (const auto& repo : tree.repos) {
            sync_repository(tree.root, repo, io_context, pool);
        }
    }
    pool.join();
    return 0;
}
