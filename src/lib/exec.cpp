#include "exec.hpp"
#include "config.hpp"
#include "output_view.hpp"
#include "repo_type.hpp"
#include "runtime.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <boost/process/v1.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> split_command(const std::string &command) {
    std::vector<std::string> args;
    std::string current;
    bool in_single_quotes = false;
    bool in_double_quotes = false;
    bool escaped = false;

    for (const char ch : command) {
        if (escaped) {
            current.push_back(ch);
            escaped = false;
            continue;
        }

        if (ch == '\\' && !in_single_quotes) {
            escaped = true;
            continue;
        }

        if (ch == '\'' && !in_double_quotes) {
            in_single_quotes = !in_single_quotes;
            continue;
        }

        if (ch == '"' && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
            continue;
        }

        if (ch == ' ' && !in_single_quotes && !in_double_quotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
            continue;
        }

        current.push_back(ch);
    }

    if (escaped || in_single_quotes || in_double_quotes) {
        return {};
    }
    if (!current.empty()) {
        args.push_back(current);
    }
    return args;
}

bool command_exists(const std::string &command) {
    namespace bp = boost::process::v1;
    const auto found = bp::search_path(command);
    return !found.empty();
}

bool starts_with_command(
    const std::vector<std::string> &command,
    const std::string &prefix_command) {
    return !command.empty() && command.front() == prefix_command;
}

std::vector<std::string>
build_command_for_repo(const std::string &custom_command, RepoType type) {
    std::vector<std::string> command_parts = split_command(custom_command);
    if (command_parts.empty()) {
        return {};
    }
    const std::string cli = repo_type_to_string(type);
    if (!cli.empty() && command_exists(cli) &&
        !starts_with_command(command_parts, cli)) {
        command_parts.insert(command_parts.begin(), cli);
    }
    return command_parts;
}

int execute_in_repo(
    const std::string &repo_path,
    const std::vector<std::string> &command_parts) {
    if (command_parts.empty()) {
        return 1;
    }

    namespace bp = boost::process::v1;
    try {
        std::vector<std::string> args(
            command_parts.begin() + 1,
            command_parts.end());
        bp::child process(
            command_parts[0],
            bp::args = args,
            bp::start_dir = repo_path,
            bp::std_out > bp::null,
            bp::std_err > bp::null);
        process.wait();
        return process.exit_code();
    } catch (const std::exception &) {
        return 1;
    }
}

std::pair<std::string, std::string>
parse_repo_path(const std::vector<Tree> &trees, const std::string &repo_path) {
    for (const auto &tree : trees) {
        const std::filesystem::path prefix =
            std::filesystem::path(tree.root) / "";
        const std::string prefix_str = prefix.string();
        if (repo_path.starts_with(prefix_str)) {
            const std::string repo_name = repo_path.substr(prefix_str.length());
            return {tree.root, repo_name};
        }
    }
    return {"", repo_path};
}
} // namespace

int run_exec(const ExecutionOptions &options) {
    const auto trees = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns);

    const ExecPlanResult plan =
        plan_exec(options.command, trees, options.repository_type);
    if (!plan.error.empty()) {
        std::cerr << plan.error << "\n";
        return 1;
    }

    Tracker tracker;
    tracker.populate(trees);

    auto view = create_output_view(
        detect_output_mode(),
        DisplayFormat::PROGRESS,
        tracker);
    view->start();

    int return_code = 0;
    for (const auto &item : plan.items) {
        const auto [root, repo_name] = parse_repo_path(trees, item.repo_path);

        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::RUNNING,
            "Executing command");

        const int command_code =
            execute_in_repo(item.repo_path, item.command_parts);

        if (command_code != 0) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::FAILED,
                "Command failed with code " + std::to_string(command_code),
                MessageLevel::ERROR);
            return_code = 1;
        } else {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::SUCCEEDED,
                "Command completed successfully");
        }
    }

    tracker.close();
    view->stop();
    return return_code;
}

ExecPlanResult plan_exec(
    const std::string &custom_command,
    const std::vector<Tree> &config,
    const std::string &repo_type) {
    std::optional<RepoType> target_repo_type;
    if (repo_type != "all") {
        target_repo_type = parse_repo_type(repo_type);
        if (!target_repo_type.has_value()) {
            return {
                .items = {},
                .error = "Invalid repo type: " + repo_type +
                         ". Expected one of: all, git, svn, hg"};
        }
    }

    std::vector<ExecPlanItem> items;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            if (target_repo_type.has_value() &&
                repo.type != *target_repo_type) {
                continue;
            }
            const std::string repo_path =
                (std::filesystem::path(tree.root) / repo.name).string();
            const std::vector<std::string> command_parts =
                build_command_for_repo(custom_command, repo.type);
            if (command_parts.empty()) {
                return {
                    .items = {},
                    .error = "Invalid command syntax: " + custom_command};
            }
            items.push_back(
                ExecPlanItem{
                    .repo_path = repo_path,
                    .command_parts = command_parts});
        }
    }
    return {.items = items, .error = ""};
}
