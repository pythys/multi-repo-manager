#include <algorithm>
#include <iostream>
#include <string>
#include "config.hpp"
#include "sync.hpp"
#include "tree.hpp"

int run_sync(const std::string& config_file) {
    std::vector<Tree> config = get_config(config_file);
    // TODO replace with parallel transform
    std::for_each(config.begin(), config.end(), [](const Tree& tree) {
        std::cout << tree.root << std::endl;
        std::cout << tree.repos.size() << std::endl;
    });
    return 0;
}
