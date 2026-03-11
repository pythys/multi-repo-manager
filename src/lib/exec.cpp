#include "exec.hpp"
#include "config.hpp"
#include "repo_type.hpp"
#include "runtime.hpp"
#include "tree.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
constexpr int kExecFailureExitCode = 127;

std::vector<std::string> split(const std::string &text, char delimiter) {
    std::vector<std::string> items;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        if (!item.empty()) {
            items.push_back(item);
        }
    }
    return items;
}

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
    const auto path_value = get_env("PATH");
    if (!path_value) {
        return false;
    }

    for (const auto &directory : split(*path_value, ':')) {
        const std::filesystem::path candidate =
            std::filesystem::path(directory) / command;
        std::error_code ec;
        const auto status = std::filesystem::status(candidate, ec);
        if (ec || !std::filesystem::is_regular_file(status)) {
            continue;
        }
        const auto permissions = status.permissions();
        const auto executable_bits = std::filesystem::perms::owner_exec |
                                     std::filesystem::perms::group_exec |
                                     std::filesystem::perms::others_exec;
        if ((permissions & executable_bits) != std::filesystem::perms::none) {
            return true;
        }
    }
    return false;
}

std::string repo_cli(RepoType type) {
    switch (type) {
    case RepoType::GIT:
        return "git";
    case RepoType::SVN:
        return "svn";
    case RepoType::HG:
        return "hg";
    default:
        return "";
    }
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
    const std::string cli = repo_cli(type);
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

    std::vector<std::vector<char>> mutable_args;
    mutable_args.reserve(command_parts.size());
    for (const auto &part : command_parts) {
        std::vector<char> buffer(part.begin(), part.end());
        buffer.push_back('\0');
        mutable_args.push_back(std::move(buffer));
    }

    std::vector<char *> argv;
    argv.reserve(command_parts.size() + 1);
    for (auto &part : mutable_args) {
        argv.push_back(part.data());
    }
    argv.push_back(nullptr);

    const pid_t child_pid = fork();
    if (child_pid < 0) {
        return 1;
    }

    if (child_pid == 0) {
        if (chdir(repo_path.c_str()) != 0) {
            _exit(kExecFailureExitCode);
        }
        execvp(argv.front(), argv.data());
        _exit(kExecFailureExitCode);
    }

    int status = 0;
    if (waitpid(child_pid, &status, 0) < 0) {
        return 1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 1;
}
} // namespace

int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type,
    const std::vector<std::string> &root_patterns) {
    const std::vector<Tree> config =
        filter_trees_by_root(get_config(config_file), root_patterns);

    const ExecPlanResult plan = plan_exec(custom_command, config, repo_type);
    if (!plan.error.empty()) {
        std::cerr << plan.error << "\n";
        return 1;
    }

    int return_code = 0;
    for (const auto &item : plan.items) {
        const int command_code =
            execute_in_repo(item.repo_path, item.command_parts);
        if (command_code != 0) {
            std::cerr << "Command failed in " << item.repo_path << ": "
                      << command_code << "\n";
            return_code = 1;
        }
    }

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
            const std::string repo_path = tree.root + "/" + repo.name;
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
