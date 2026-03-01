#include "status.hpp"
#include "config.hpp"
#include "repo_factory.hpp"
#include "tree.hpp"
#include <iostream>
#include <string>
#include <vector>

int run_status(
    const std::string &config_file,
    const std::vector<std::string> &root_patterns) {
    const std::vector<Tree> config =
        filter_trees_by_root(get_config(config_file), root_patterns);
    for (const auto &tree : config) {
        std::cout << "\n";
        for (const auto &repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = tree.root + "/" + repo.name;
            std::cout << "Status for " << repo_path << ":\n";
            try {
                const auto statuses = repo_manager->get_status(repo_path);
                for (const auto &status : statuses) {
                    std::cout << status << "\n";
                }
            } catch (const std::exception &e) {
                std::cerr << "Error getting status for " << repo_path << ": "
                          << e.what() << "\n";
            }
            std::cout << "\n";
        }
    }
    return 0;
}
