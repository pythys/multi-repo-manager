#include <iostream>
#include <string>
#include <vector>
#include "config.hpp"
#include "status.hpp"

int run_status(const std::string& config_file) {
    std::vector<Tree> config = get_dependencies(config_file);
    return 0;
}
