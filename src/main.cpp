#include <iostream>
#include <string>
#include <CLI/CLI.hpp>
#include "sync.hpp"

int main(int argc, char **argv) {
    CLI::App app("multi-repo-manager");

    CLI::App *sync = app.add_subcommand(
        "sync",
        "Sync local repositories with a configured list"
    );

    std::string config_file = "default.yaml";
    sync->add_option(
        "--config,-c",
        config_file,
        "Configuration file"
    );

    CLI::App *find = app.add_subcommand(
        "find",
        "Generate a configuration from existing repositories"
    );

    CLI11_PARSE(app, argc, argv);

    if (*sync) {
        std::cout << "Running sync with config: "
                  << config_file
                  << "\n";
        runSync(config_file);
    }

    if (*find) {
        std::cout << "Running find\n";
        // TODO
    }

    return 0;
}
