#include <yaml-cpp/yaml.h>
#include <iostream>
#include "config.hpp"

YAML::Node get_config(const std::string& config_file) {
    YAML::Node config;
    try {
        config = YAML::LoadFile(config_file);
    } catch (YAML::Exception &e) {
        std::cerr << "Error loading YAML file: " << e.what() << std::endl;
    }
    return config;
}
