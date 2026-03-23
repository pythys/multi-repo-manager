#include "cli.hpp"
#include "command_options.hpp"
#include "completion.hpp"
#include "config.hpp"
#include "exec.hpp"
#include "find.hpp"
#include "list.hpp"
#include "remotesync.hpp"
#include "status.hpp"
#include "sync.hpp"
#include "update.hpp"
#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int parse_cli(int argc, char **argv) {
    CLI::App app("multi-repo-manager", "mrm");
    std::string config_file = "mrm.yml";

    CLI::App *sync = app.add_subcommand("sync", "Synchronize repositories");
    sync->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    int sync_jobs = 0;
    sync->add_option("--jobs,-j", sync_jobs, "number of jobs")->type_name("N");
    bool prune_remotes = false;
    sync->add_flag("--prune-remotes,-R", prune_remotes, "prune remotes");
    bool prune_branches = false;
    sync->add_flag("--prune-branches,-B", prune_branches, "prune branches");
    bool prune_all = false;
    sync->add_flag("--prune,-p", prune_all, "prune all");
    std::vector<std::string> sync_root_patterns;
    sync->add_option("--root,-r", sync_root_patterns, "root tree pattern")
        ->type_name("pattern");

    CLI::App *list = app.add_subcommand("list", "List repositories");
    list->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    std::vector<std::string> list_find_paths;
    list->add_option("--find,-f", list_find_paths, "find repositories in paths")
        ->type_name("dir");
    std::vector<std::string> list_root_patterns;
    list->add_option("--root,-r", list_root_patterns, "root tree pattern")
        ->type_name("pattern");
    std::vector<std::string> list_name_patterns;
    list->add_option(
            "--name,-n",
            list_name_patterns,
            "filter by repository name pattern")
        ->type_name("pattern");

    CLI::App *find = app.add_subcommand("find", "Find repositories");
    std::vector<std::string> find_paths;
    find->add_option("paths", find_paths, "paths")->type_name("dir");
    std::string save_path;
    CLI::Option *save_option =
        find->add_option("--save,-s", save_path, "save results to file")
            ->type_name("file")
            ->expected(0, 1);

    CLI::App *status = app.add_subcommand("status", "Show repository status");
    status->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    std::vector<std::string> status_find_paths;
    status
        ->add_option(
            "--find,-f",
            status_find_paths,
            "find repositories in paths")
        ->type_name("dir");
    std::vector<std::string> status_root_patterns;
    status->add_option("--root,-r", status_root_patterns, "root tree pattern")
        ->type_name("pattern");
    std::vector<std::string> status_name_patterns;
    status
        ->add_option(
            "--name,-n",
            status_name_patterns,
            "filter by repository name pattern")
        ->type_name("pattern");

    CLI::App *update = app.add_subcommand("update", "Update repositories");
    update->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    std::vector<std::string> update_find_paths;
    update
        ->add_option(
            "--find,-f",
            update_find_paths,
            "find repositories in paths")
        ->type_name("dir");
    int update_jobs = 0;
    update->add_option("--jobs,-j", update_jobs, "number of jobs")
        ->type_name("N");
    std::vector<std::string> update_root_patterns;
    update->add_option("--root,-r", update_root_patterns, "root tree pattern")
        ->type_name("pattern");
    std::vector<std::string> update_name_patterns;
    update
        ->add_option(
            "--name,-n",
            update_name_patterns,
            "filter by repository name pattern")
        ->type_name("pattern");

    CLI::App *remotesync =
        app.add_subcommand("remotesync", "Sync between remotes");
    remotesync->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    std::vector<std::string> remotesync_find_paths;
    remotesync
        ->add_option(
            "--find,-f",
            remotesync_find_paths,
            "find repositories in paths")
        ->type_name("dir");
    std::string remotesync_source;
    remotesync->add_option("--source,-s", remotesync_source, "source remote")
        ->required()
        ->type_name("name");
    std::string remotesync_target;
    remotesync->add_option("--target,-t", remotesync_target, "target remote")
        ->required()
        ->type_name("name");
    std::vector<std::string> remotesync_branches;
    remotesync->add_option("--branch,-b", remotesync_branches, "branch name")
        ->required()
        ->type_name("name");
    bool remotesync_dry_run = false;
    remotesync->add_flag(
        "--dry-run,-d",
        remotesync_dry_run,
        "perform a dry run");
    int remotesync_jobs = 0;
    remotesync->add_option("--jobs,-j", remotesync_jobs, "number of jobs")
        ->type_name("N");
    std::vector<std::string> remotesync_root_patterns;
    remotesync
        ->add_option("--root,-r", remotesync_root_patterns, "root tree pattern")
        ->type_name("pattern");
    std::vector<std::string> remotesync_name_patterns;
    remotesync
        ->add_option(
            "--name,-n",
            remotesync_name_patterns,
            "filter by repository name pattern")
        ->type_name("pattern");

    CLI::App *exec =
        app.add_subcommand("exec", "Execute command in repositories");
    std::string custom_command;
    exec->add_option("--command,-m", custom_command, "command to run")
        ->required()
        ->type_name("command");
    std::string repo_type = "all";
    exec->add_option("--type,-t", repo_type, "repository type")
        ->type_name("type")
        ->check(CLI::IsMember({"all", "git", "svn", "hg"}));
    exec->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    std::vector<std::string> exec_find_paths;
    exec->add_option("--find,-f", exec_find_paths, "find repositories in paths")
        ->type_name("dir");
    std::vector<std::string> exec_root_patterns;
    exec->add_option("--root,-r", exec_root_patterns, "root tree pattern")
        ->type_name("pattern");
    std::vector<std::string> exec_name_patterns;
    exec->add_option(
            "--name,-n",
            exec_name_patterns,
            "filter by repository name pattern")
        ->type_name("pattern");

    CLI::App *completion =
        app.add_subcommand("completion", "Generate completion");
    std::string completion_shell;
    completion->add_option("shell", completion_shell, "Shell type")
        ->required()
        ->check(CLI::IsMember({"bash", "zsh", "spec"}));

    if (argc <= 1) {
        return app.exit(CLI::CallForHelp());
    }

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    const bool needs_input =
        *sync || *status || *update || *remotesync || *exec || *list;

    const bool has_find = !status_find_paths.empty() ||
                          !update_find_paths.empty() ||
                          !remotesync_find_paths.empty() ||
                          !exec_find_paths.empty() || !list_find_paths.empty();

    if (*sync && has_find) {
        std::cerr << "Error: sync command does not support --find.\n";
        std::cerr << "Use --config instead.\n";
        return 1;
    }

    std::error_code error;
    const bool has_config_file = std::filesystem::exists(config_file, error);
    if (needs_input && !has_find && !has_config_file) {
        std::cerr << "Config file not found: " << config_file << "\n";
        std::cerr << "Use --config <file>, --find <paths>, or run `mrm find "
                     "--save` to create one.\n";
        return 1;
    }

    if (*sync) {
        const bool should_prune_remotes = prune_all || prune_remotes;
        const bool should_prune_branches = prune_all || prune_branches;
        SyncOptions options{
            .config_file = config_file,
            .root_patterns = sync_root_patterns,
            .prune_remotes = should_prune_remotes,
            .prune_branches = should_prune_branches,
            .jobs = sync_jobs};
        return run_sync(options);
    }

    if (*list) {
        ListOptions options{
            .selector = {
                .config_file = config_file,
                .find_paths = list_find_paths,
                .root_patterns = list_root_patterns,
                .name_patterns = list_name_patterns}};
        return run_list(options);
    }

    if (*find) {
        if (save_option->count() > 0 && save_path.empty()) {
            save_path = "mrm.yml";
        }
        return run_find(find_paths, save_path);
    }

    if (*status) {
        StatusOptions options{
            .selector = {
                .config_file = config_file,
                .find_paths = status_find_paths,
                .root_patterns = status_root_patterns,
                .name_patterns = status_name_patterns}};
        return run_status(options);
    }

    if (*update) {
        UpdateOptions options{
            .selector =
                {.config_file = config_file,
                 .find_paths = update_find_paths,
                 .root_patterns = update_root_patterns,
                 .name_patterns = update_name_patterns},
            .jobs = update_jobs};
        return run_update(options);
    }

    if (*remotesync) {
        RemoteSyncOptions options{
            .selector =
                {.config_file = config_file,
                 .find_paths = remotesync_find_paths,
                 .root_patterns = remotesync_root_patterns,
                 .name_patterns = remotesync_name_patterns},
            .source_remote = remotesync_source,
            .target_remote = remotesync_target,
            .branches = remotesync_branches,
            .dry_run = remotesync_dry_run,
            .jobs = remotesync_jobs};
        return run_remotesync(options);
    }

    if (*exec) {
        ExecutionOptions options{
            .selector =
                {.config_file = config_file,
                 .find_paths = exec_find_paths,
                 .root_patterns = exec_root_patterns,
                 .name_patterns = exec_name_patterns},
            .command = custom_command,
            .repository_type = repo_type};
        return run_exec(options);
    }

    if (*completion) {
        try {
            std::cout << generate_script(app, completion_shell);
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << "\n";
            return 1;
        }
        return 0;
    }

    return 0;
}
