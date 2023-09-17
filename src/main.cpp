#include <string>
#include <CLI/CLI.hpp>
#include "sync.hpp"
#include "find.hpp"

int main(int argc, char **argv) {
    CLI::App app("multi-repo-manager");

    CLI::App *sync = app.add_subcommand(
        "sync",
        "Sync local repositories with a configured list"
    );

    std::string config_file;
    sync->add_option(
        "--config,-c",
        config_file,
        "Configuration file"
    )->required()->type_name("file");;

    std::string sync_path = ".";
    sync->add_option(
        "workdir",
        sync_path,
        "Path to sync repos to if location is relative. Defaults to \".\""
    )->type_name("dir");

    CLI::App *find = app.add_subcommand(
        "find",
        "Generate a configuration from existing repositories"
    );

    std::string find_path = ".";
    find->add_option(
        "path",
        find_path,
        "Path to search for existing repositories. Defaults to \".\""
    )->type_name("dir");

    CLI11_PARSE(app, argc, argv);

    if (*sync) {
        runSync(config_file);
    }

    if (*find) {
        runFind(find_path);
    }

    return 0;
}
