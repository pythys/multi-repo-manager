#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "repo_factory.hpp"
#include "status.hpp"

int run_status(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    for (auto tree : config) {
        std::cout << std::endl;
        for (auto repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = tree.root + "/" + repo.name;
            const auto statuses = repo_manager->get_status(repo_path);
            std::cout << "Status for " << repo_path << ":" << std::endl;
            for (const auto& status : statuses) {
                std::cout << status << std::endl;
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
