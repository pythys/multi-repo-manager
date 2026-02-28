#include "cli.hpp"
#include "exec.hpp"
#include "find.hpp"
#include "status.hpp"
#include "sync.hpp"
#include "update.hpp"
#include <CLI/CLI.hpp>
#include <string>

int parse_cli(int argc, char **argv) {
    CLI::App app("mrm");
    std::string config_file;

    CLI::App *sync = app.add_subcommand(
        "sync",
        "Sync local repositories with a configured list");
    sync->add_option("--config,-c", config_file, "Configuration file")
        ->required()
        ->type_name("file");
    std::string sync_path = ".";
    sync->add_option(
            "workdir",
            sync_path,
            "Path to sync repos to if location is relative. Defaults to \".\"")
        ->type_name("dir");
    int sync_jobs = 0;
    sync->add_option(
            "--jobs,-j",
            sync_jobs,
            "Max concurrent repo operations. Use 0 (default) to use the "
            "built-in value")
        ->type_name("N");
    sync->add_option("--pool-size", sync_jobs, "DEPRECATED: use --jobs")
        ->type_name("N")
        ->group("");
    bool prune_remotes = false;
    sync->add_flag(
        "--prune-remotes",
        prune_remotes,
        "Remove remotes not declared in config");
    bool prune_branches = false;
    sync->add_flag(
        "--prune-branches",
        prune_branches,
        "Remove local tracked branches not declared in config");
    bool prune_all = false;
    sync->add_flag(
        "--prune",
        prune_all,
        "Enable both --prune-remotes and --prune-branches");

    CLI::App *find = app.add_subcommand(
        "find",
        "Generate a configuration from existing repositories");
    std::string find_path = ".";
    find->add_option(
            "path",
            find_path,
            "Path to search for existing repositories. Defaults to \".\"")
        ->type_name("dir");
    std::string save_path;
    find->add_option("--save,-s", save_path, "Save to a file instead of stdout")
        ->type_name("file");

    CLI::App *status =
        app.add_subcommand("status", "Show the status of config repositories");
    status
        ->add_option(
            "--config,-c",
            config_file,
            "Configuration file for the status command")
        ->required()
        ->type_name("file");

    CLI::App *update =
        app.add_subcommand("update", "Update config repositories");
    update
        ->add_option(
            "--config,-c",
            config_file,
            "Configuration file for the update command")
        ->required()
        ->type_name("file");
    int update_jobs = 0;
    update
        ->add_option(
            "--jobs,-j",
            update_jobs,
            "Max concurrent repo operations. Use 0 (default) to use the "
            "built-in value")
        ->type_name("N");
    update->add_option("--pool-size", update_jobs, "DEPRECATED: use --jobs")
        ->type_name("N")
        ->group("");

    CLI::App *remotesync =
        app.add_subcommand("remotesync", "Sync remotes with origin");
    remotesync->add_option("--config,-c", config_file, "Configuration file")
        ->required()
        ->type_name("file");

    CLI::App *exec = app.add_subcommand(
        "exec",
        "Execute a custom command on repositories of a certain type");
    std::string custom_command;
    exec->add_option(
            "--command,-m",
            custom_command,
            "The custom command to run")
        ->required()
        ->type_name("command");
    std::string repo_type = "all";
    exec->add_option(
            "--type,-t",
            repo_type,
            "Type of repositories to target. Defaults to 'all'")
        ->type_name("type");
    exec->add_option(
            "--config,-c",
            config_file,
            "Configuration file for the exec command")
        ->required()
        ->type_name("file");

    CLI11_PARSE(app, argc, argv);

    if (*sync) {
        const bool should_prune_remotes = prune_all || prune_remotes;
        const bool should_prune_branches = prune_all || prune_branches;
        return run_sync(
            config_file,
            sync_jobs,
            should_prune_remotes,
            should_prune_branches);
    }

    if (*find) {
        return run_find(find_path, save_path);
    }

    if (*status) {
        return run_status(config_file);
    }

    if (*update) {
        return run_update(config_file, update_jobs);
    }

    if (*remotesync) {
        return 0;
    }

    if (*exec) {
        return run_exec(custom_command, config_file, repo_type);
    }

    return 0;
}
