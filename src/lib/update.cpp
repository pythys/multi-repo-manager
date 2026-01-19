#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "config.hpp"
#include "constants.hpp"
#include "repo_factory.hpp"
#include "update.hpp"

namespace asio = boost::asio;

int run_update(const std::string& config_file) {
    const std::vector<Tree> config = get_config(config_file);
    asio::thread_pool pool(SYNC_POOL_SIZE);
    for (const auto& tree : config) {
        for (const auto& repo : tree.repos) {
            auto updater = [repo, tree] {
                auto repo_manager = create_repo_manager(repo.type);
                auto repo_path = tree.root + "/" + repo.name;
                std::cout << "Updating " << repo_path << '\n';
                try {
                    repo_manager->update(repo_path);
                } catch (const std::exception& e) {
                    std::cerr << "Error updating "
                              << repo_path
                              << ": "
                              << e.what()
                              << "\n";
                    return;
                }
                std::cout << "Finished updating " << repo_path << '\n';
            };
            asio::post(pool, updater);
        }
    }
    pool.join();
    return 0;
}
