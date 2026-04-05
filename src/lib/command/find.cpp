#include "command/find.hpp"
#include "core/tree.hpp"
#include "persistence/config.hpp"
#include "persistence/discovery.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int run_find(
    const std::vector<std::string> &find_paths,
    const std::string &save_path) {
    std::vector<std::string> roots = find_paths;
    if (roots.empty()) {
        roots.emplace_back(".");
    }

    std::vector<Tree> trees;
    for (const auto &path : roots) {
        const std::string normalized_root = normalize_path(path);
        trees.push_back(
            Tree{
                .root = normalized_root,
                .repos = find_repos(normalized_root)});
    }

    std::string config_output = make_config(trees);
    if (!save_path.empty()) {
        std::ofstream file(save_path);
        if (config_output.empty() || config_output.back() != '\n') {
            config_output += '\n';
        }
        file << config_output;
        file.close();
        std::cout << "Config saved to " << save_path << "\n";
    } else {
        std::cout << config_output << "\n";
    }
    return 0;
}
