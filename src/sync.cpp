#include <algorithm>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include "config.hpp"
#include "sync.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_config(config_file);
    /* TODO implement parallel algorithm as follows
     * is path a directory and a repo ?
     * - true: update the remotes
     * - false: clone the repository
     */
    std::ranges::for_each(config, [](const Tree& tree) {
        std::ranges::for_each(tree.repos, [&tree](const auto& repo) {
            fs::path repo_dir(tree.root + "/" + repo.name);
            if (fs::exists(repo_dir) && fs::is_directory(repo_dir)) {
                std::cout << "path exists" << std::endl;
            } else {
                std::cout << "path does not exist" << std::endl;
            }
        });
    });
    return 0;
}
