#include "command/exec.hpp"
#include "core/tracker.hpp"
#include "core/tree.hpp"
#include "persistence/config.hpp"
#include "presentation/output_view.hpp"
#include "presentation/tracked_operation.hpp"
#include "util/common.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/process/v1.hpp>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace asio = boost::asio;

namespace {

std::string substitute_placeholders(
    const std::string &command,
    const std::string &repo_path,
    const std::string &repo_name,
    const std::string &tree_root) {
    std::string result = command;
    result = replace_all(result, "{path}", repo_path);
    result = replace_all(result, "{name}", repo_name);
    result = replace_all(result, "{root}", tree_root);
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

struct ExecResult {
    int exit_code;
    std::vector<std::string> output_lines;
};

ExecResult
execute_in_repo(const std::string &repo_path, const std::string &full_command) {
    if (full_command.empty()) {
        return {.exit_code = 1, .output_lines = {}};
    }

    namespace bp = boost::process::v1;
    try {
        boost::asio::io_context ioc;
        std::future<std::string> stdout_data;
        std::future<std::string> stderr_data;

        bp::child process(
            full_command,
            bp::start_dir = repo_path,
            bp::std_out > stdout_data,
            bp::std_err > stderr_data,
            bp::shell,
            ioc);

        ioc.run();
        process.wait();

        std::vector<std::string> output_lines;
        std::istringstream stdout_stream(stdout_data.get());
        std::istringstream stderr_stream(stderr_data.get());

        std::string line;
        while (std::getline(stdout_stream, line)) {
            output_lines.push_back(line);
        }
        while (std::getline(stderr_stream, line)) {
            output_lines.push_back(line);
        }

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
        options.selector.name_patterns,
        options.selector.min_depth);

    const ExecPlanResult plan = plan_exec(options.command, trees);
    if (!plan.error.empty()) {
        std::cerr << plan.error << "\n";
        return 1;
    }

    TrackedOperation op(trees, DisplayFormat::PROGRESS);
    auto &tracker = op.tracker();

    std::atomic_bool has_error{false};
    auto pool = create_thread_pool(options.jobs);
    for (const auto &item : plan.items) {
        auto executor = [item, &trees, &tracker, &has_error] {
            const auto [root, repo_name] =
                parse_repo_path(trees, item.repo_path);

            tracker.set_phase(
                root,
                repo_name,
                RepoPhase::RUNNING,
                "Executing command");

            const ExecResult result =
                execute_in_repo(item.repo_path, item.command);

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
                    "Command failed with code " +
                        std::to_string(result.exit_code),
                    MessageLevel::ERROR);
                has_error.store(true);
            } else {
                tracker.set_phase(
                    root,
                    repo_name,
                    RepoPhase::SUCCEEDED,
                    "Command completed successfully");
            }
        };
        asio::post(pool, executor);
    }
    pool.join();
    return has_error.load() ? 1 : 0;
}

ExecPlanResult
plan_exec(const std::string &custom_command, const std::vector<Tree> &config) {
    std::vector<ExecPlanItem> items;
    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            const std::string repo_path =
                construct_repo_path(tree.root, repo.name);
            const std::string substituted = substitute_placeholders(
                custom_command,
                repo_path,
                repo.name,
                tree.root);
            if (split_command(substituted).empty()) {
                return {
                    .items = {},
                    .error = "Invalid command syntax: " + custom_command};
            }
            items.push_back(
                ExecPlanItem{.repo_path = repo_path, .command = substituted});
        }
    }
    return {.items = items, .error = ""};
}
