#ifndef SRC_LIB_EXEC_HPP_
#define SRC_LIB_EXEC_HPP_

/**
 * @file exec.hpp
 * @brief Exec command entry point.
 */

#include "tree.hpp"
#include <string>
#include <vector>

/** One planned command invocation for a repository path. */
struct ExecPlanItem {
    std::string repo_path;
    std::vector<std::string> command_parts;
};

/** Result of planning exec operations. */
struct ExecPlanResult {
    std::vector<ExecPlanItem> items;
    std::string error;
};

/**
 * @brief Build repository command invocations without executing them.
 *
 * @return `error` is non-empty when planning fails.
 */
ExecPlanResult plan_exec(
    const std::string &custom_command,
    const std::vector<Tree> &config,
    const std::string &repo_type);

/**
 * @brief Execute custom command over configured repositories.
 *
 * Runs the command in each targeted repository. When the SCM CLI for the
 * repository type is available (for example, `git`), it is prepended unless
 * the command already starts with that CLI.
 *
 * @return 0 when all targeted repositories succeed, otherwise 1.
 */
int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_EXEC_HPP_
