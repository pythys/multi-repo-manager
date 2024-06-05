#include <filesystem>
#include <iostream>
#include <string>
#include "find.hpp"

namespace fs = std::filesystem;
int run_find(const std::string& path) {
    std::cout << "Running find with path: "
              << path
              << std::endl;
    return 0;
}
