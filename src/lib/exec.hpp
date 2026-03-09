#ifndef SRC_LIB_EXEC_HPP_
#define SRC_LIB_EXEC_HPP_

#include "tree.hpp"
#include <string>
#include <vector>

struct ExecPlanItem {
    std::string repo_path;
    std::vector<std::string> command_parts;
};

struct ExecPlanResult {
    std::vector<ExecPlanItem> items;
    std::string error;
};

ExecPlanResult plan_exec(
    const std::string &custom_command,
    const std::vector<Tree> &config,
    const std::string &repo_type);

int run_exec(
    const std::string &custom_command,
    const std::string &config_file,
    const std::string &repo_type,
    const std::vector<std::string> &root_patterns = {});

#endif // SRC_LIB_EXEC_HPP_
