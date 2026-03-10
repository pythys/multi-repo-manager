#include "cli.hpp"
#include "completion.hpp"
#include "exec.hpp"
#include "find.hpp"
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
    sync->add_flag("--prune,-p", prune_all, "prune both");
    std::vector<std::string> sync_root_patterns;
    sync->add_option("--root,-r", sync_root_patterns, "root directory")
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
    std::vector<std::string> status_root_patterns;
    status->add_option("--root,-r", status_root_patterns, "root directory")
        ->type_name("pattern");

    CLI::App *update = app.add_subcommand("update", "Update repositories");
    update->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
    int update_jobs = 0;
    update->add_option("--jobs,-j", update_jobs, "number of jobs")
        ->type_name("N");
    std::vector<std::string> update_root_patterns;
    update->add_option("--root,-r", update_root_patterns, "root directory")
        ->type_name("pattern");

    CLI::App *remotesync =
        app.add_subcommand("remotesync", "Sync between remotes");
    remotesync->add_option("--config,-c", config_file, "config file")
        ->type_name("file");
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
        "--dry-run,-n",
        remotesync_dry_run,
        "perform a dry run");
    int remotesync_jobs = 0;
    remotesync->add_option("--jobs,-j", remotesync_jobs, "number of jobs")
        ->type_name("N");
    std::vector<std::string> remotesync_root_patterns;
    remotesync
        ->add_option("--root,-r", remotesync_root_patterns, "root directory")
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
    std::vector<std::string> exec_root_patterns;
    exec->add_option("--root,-r", exec_root_patterns, "root directory")
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

    const bool needs_config =
        *sync || *status || *update || *remotesync || *exec;
    std::error_code error;
    const bool has_config_file = std::filesystem::exists(config_file, error);
    if (needs_config && !has_config_file) {
        std::cerr << "Config file not found: " << config_file << "\n";
        std::cerr << "Use --config <file> or run `mrm find --save` to "
                     "create one.\n";
        return 1;
    }

    if (*sync) {
        const bool should_prune_remotes = prune_all || prune_remotes;
        const bool should_prune_branches = prune_all || prune_branches;
        return run_sync(
            config_file,
            sync_jobs,
            should_prune_remotes,
            should_prune_branches,
            sync_root_patterns);
    }

    if (*find) {
        if (save_option->count() > 0 && save_path.empty()) {
            save_path = "mrm.yml";
        }
        return run_find(find_paths, save_path);
    }

    if (*status) {
        return run_status(config_file, status_root_patterns);
    }

    if (*update) {
        return run_update(config_file, update_jobs, update_root_patterns);
    }

    if (*remotesync) {
        return run_remotesync(
            config_file,
            remotesync_source,
            remotesync_target,
            remotesync_branches,
            remotesync_dry_run,
            remotesync_root_patterns,
            remotesync_jobs);
    }

    if (*exec) {
        return run_exec(
            custom_command,
            config_file,
            repo_type,
            exec_root_patterns);
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
