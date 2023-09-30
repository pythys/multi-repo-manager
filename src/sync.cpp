#include <algorithm>
#include <filesystem>
#include <iostream>
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
    std::for_each(config.begin(), config.end(), [](const Tree& tree) {
        std::for_each(tree.repos.begin(), tree.repos.end(), [&tree](const auto& repo) {
            fs::path p(tree.root + "/" + repo.name);
            if (fs::exists(p) && fs::is_directory(p)) {
                std::cout << "path exists" << std::endl;
            } else {
                std::cout << "path does not exit" << std::endl;
            }
        });
    });
    return 0;
}
