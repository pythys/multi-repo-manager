#include "config.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <algorithm>

Remote to_remote(const YAML::Node& node) {
    return {
        node["name"].as<std::string>(),
        node["url"].as<std::string>(),
        node["type"].as<std::string>()
    };
}

Repo to_repo(const YAML::Node& node) {
    std::vector<Remote> remotes;
    if (node["remotes"] && node["remotes"].IsSequence()) {
        std::transform(
            node["remotes"].begin(),
            node["remotes"].end(),
            std::back_inserter(remotes),
            to_remote
        );
    } else {
        std::cerr << "Warning: 'remotes' node is missing or not a sequence for repo "
                  << node["name"].as<std::string>("unknown")
                  << std::endl;
    }
    return {
        node["name"].as<std::string>(),
        node["type"].as<std::string>(),
        remotes
    };
}

Tree to_tree(const YAML::Node& node) {
    std::vector<Repo> repos;
    std::transform(
        node["repos"].begin(),
        node["repos"].end(),
        std::back_inserter(repos),
        to_repo
    );
    return {node["root"].as<std::string>(), repos};
}

std::vector<Tree> get_config(const std::string& config_file) {
    std::vector<Tree> trees;
    try {
        YAML::Node config = YAML::LoadFile(config_file);

        if (config["trees"]) {
            std::transform(
                config["trees"].begin(),
                config["trees"].end(),
                std::back_inserter(trees),
                to_tree
            );
        } else {
            std::cerr << "Error: 'trees' node is missing."
                      << std::endl;
        }
    } catch (YAML::Exception &e) {
        std::cerr << "Error loading YAML file: "
                  << e.what()
                  << std::endl;
    }
    return trees;
}
