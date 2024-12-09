#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "repo_factory.hpp"
#include "status.hpp"

int run_status(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    for (auto tree : config) {
        for (auto repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            const auto status = repo_manager->get_status(
                tree.root + "/" + repo.name);
            std::cout << status << std::endl;
        }
    }
    return 0;
}
