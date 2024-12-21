#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "repo_factory.hpp"
#include "update.hpp"

int run_update(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    for (auto tree : config) {
        for (auto repo : tree.repos) {
            auto repo_manager = create_repo_manager(repo.type);
            auto repo_path = tree.root + "/" + repo.name;
            repo_manager->update(repo_path);
        }
    }
    return 0;
}
