#include "exec.hpp"
#include "config.hpp"
#include "repo_type.hpp"
#include "tree.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string shell_quote(const std::string &text) {
    std::string out = "'";
    for (const char ch : text) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
}

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

bool command_exists(const std::string &command) {
    const char *path_value = std::getenv("PATH");
    if (!path_value) {
        return false;
    }

    for (const auto &directory : split(path_value, ':')) {
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
    const std::string &command,
    const std::string &prefix_command) {
    if (command == prefix_command) {
        return true;
    }
    const std::string prefix_with_space = prefix_command + " ";
    return command.starts_with(prefix_with_space);
}

std::string build_command_for_repo(
    const std::string &custom_command,
    const std::string &repo_path,
    RepoType type) {
    const std::string cli = repo_cli(type);
    std::string effective_command = custom_command;
    if (!cli.empty() && command_exists(cli) &&
        !starts_with_command(custom_command, cli)) {
        effective_command = cli + " " + custom_command;
    }
    return "cd " + shell_quote(repo_path) + " && " + effective_command;
}
} // namespace

int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type) {
    const std::vector<Tree> config = get_config(config_file);

    std::optional<RepoType> target_repo_type;
    if (repo_type != "all") {
        target_repo_type = parse_repo_type(repo_type);
        if (!target_repo_type.has_value()) {
            std::cerr << "Invalid repo type: " << repo_type
                      << ". Expected one of: all, git, svn, hg\n";
            return 1;
        }
    }

    int return_code = 0;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            if (target_repo_type.has_value() &&
                repo.type != *target_repo_type) {
                continue;
            }
            const std::string repo_path = tree.root + "/" + repo.name;
            const std::string command =
                build_command_for_repo(custom_command, repo_path, repo.type);
            const int command_code = std::system(command.c_str());
            if (command_code != 0) {
                std::cerr << "Command failed in " << repo_path << ": "
                          << command_code << "\n";
                return_code = 1;
            }
        }
    }

    return return_code;
}
