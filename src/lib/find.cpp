#include <filesystem>
#include <iostream>
#include <string>
#include "find.hpp"
#include "tree.hpp"

namespace fs = std::filesystem;

int run_find(const std::string& path) {
    fs::path root(path);
    if (fs::exists(root) && fs::is_directory(root)) {
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            std::cout << entry.path() << std::endl;
        }
    }
    return 0;
}
