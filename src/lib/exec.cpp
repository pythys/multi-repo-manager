#include "exec.hpp"
#include "config.hpp"
#include "output_view.hpp"
#include "repo_type.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include "utils.hpp"
#include <boost/process/v1.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string
replace_all(std::string str, const std::string &from, const std::string &to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

std::string substitute_placeholders(
    const std::string &command,
    const std::string &repo_path,
    const std::string &repo_name,
    const std::string &tree_root,
    RepoType repo_type) {
    std::string result = command;
    result = replace_all(result, "{path}", repo_path);
    result = replace_all(result, "{name}", repo_name);
    result = replace_all(result, "{root}", tree_root);
    result = replace_all(result, "{type}", repo_type_to_string(repo_type));
    return result;
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

std::vector<std::string> build_command_for_repo(
    const std::string &custom_command,
    const std::string &repo_path,
    const std::string &repo_name,
    const std::string &tree_root,
    RepoType repo_type) {
    const std::string substituted = substitute_placeholders(
        custom_command,
        repo_path,
        repo_name,
        tree_root,
        repo_type);
    return split_command(substituted);
}

struct ExecResult {
    int exit_code;
    std::vector<std::string> output_lines;
};

ExecResult execute_in_repo(
    const std::string &repo_path,
    const std::vector<std::string> &command_parts) {
    if (command_parts.empty()) {
        return {.exit_code = 1, .output_lines = {}};
    }

    namespace bp = boost::process::v1;
    try {
        std::ostringstream command_stream;
        command_stream << command_parts[0];
        for (size_t i = 1; i < command_parts.size(); ++i) {
            command_stream << " " << command_parts[i];
        }
        const std::string full_command = command_stream.str();

        bp::ipstream stdout_stream;
        bp::ipstream stderr_stream;
        bp::child process(
            full_command,
            bp::start_dir = repo_path,
            bp::std_out > stdout_stream,
            bp::std_err > stderr_stream,
            bp::shell);

        std::vector<std::string> output_lines;
        std::string line;
        while (std::getline(stdout_stream, line)) {
            output_lines.push_back(line);
        }
        while (std::getline(stderr_stream, line)) {
            output_lines.push_back(line);
        }

        process.wait();
        return {.exit_code = process.exit_code(), .output_lines = output_lines};
    } catch (const std::exception &) {
        return {.exit_code = 1, .output_lines = {}};
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

    TrackedOperation op(trees, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    int return_code = 0;
    for (const auto &item : plan.items) {
        const auto [root, repo_name] = parse_repo_path(trees, item.repo_path);

        tracker.set_phase(
            root,
            repo_name,
            RepoPhase::RUNNING,
            "Executing command");

        const ExecResult result =
            execute_in_repo(item.repo_path, item.command_parts);

        for (const auto &line : result.output_lines) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                line,
                MessageLevel::OUTPUT);
        }

        if (result.exit_code != 0) {
            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::FAILED,
                "Command failed with code " + std::to_string(result.exit_code),
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
                construct_repo_path(tree.root, repo.name);
            const std::vector<std::string> command_parts =
                build_command_for_repo(
                    custom_command,
                    repo_path,
                    repo.name,
                    tree.root,
                    repo.type);
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
