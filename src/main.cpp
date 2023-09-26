#include <CLI/CLI.hpp>
#include <string>
#include "exec.hpp"
#include "find.hpp"
#include "sync.hpp"

int main(int argc, char **argv) {
    CLI::App app("mrm");

    CLI::App *sync = app.add_subcommand(
        "sync",
        "Sync local repositories with a configured list"
    );

    std::string config_file;
    sync->add_option(
        "--config,-c",
        config_file,
        "Configuration file"
    )->required()->type_name("file");

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

    CLI::App *exec = app.add_subcommand(
        "exec",
        "Execute a custom command on repositories of a certain type"
    );

    std::string custom_command;
    exec->add_option(
        "--exec,-x",
        custom_command,
        "The custom command to run"
    )->required()->type_name("command");

    std::string repo_type = "all";
    exec->add_option(
        "--type,-t",
        repo_type,
        "Type of repositories to target. Defaults to 'all'"
    )->type_name("type");

    exec->add_option(
        "--config,-c",
        config_file,
        "Configuration file for the exec command"
    )->required()->type_name("file");

    CLI11_PARSE(app, argc, argv);

    if (*sync) {
        return run_sync(config_file);
    }

    if (*find) {
        return run_find(find_path);
    }

    if (*exec) {
        return run_exec(custom_command, config_file, repo_type);
    }

    return 0;
}
