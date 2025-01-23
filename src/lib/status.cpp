#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "repo_factory.hpp"
#include "status.hpp"
#include "tree.hpp"

int run_status(const std::string& config_file) {
    const std::vector<Tree> config = get_config(config_file);
    for (const auto& tree : config) {
        std::cout << "\n";
        for (const auto& repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = tree.root + "/" + repo.name;
            const auto statuses = repo_manager->get_status(repo_path);
            std::cout << "Status for " << repo_path << ":\n";
            for (const auto& status : statuses) {
                std::cout << status << "\n";
            }
            std::cout << "\n";
        }
    }
    return 0;
}
