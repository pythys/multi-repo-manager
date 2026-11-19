#ifndef SRC_LIB_UTIL_COMMAND_OPTIONS_HPP_
#define SRC_LIB_UTIL_COMMAND_OPTIONS_HPP_

#include "constants.hpp"
#include <string>
#include <vector>

struct RepositorySelector {
    std::string config_file;
    std::vector<std::string> find_paths;
    std::vector<std::string> root_patterns;
    std::vector<std::string> name_patterns;
    int min_depth = 0;
    int max_depth = UNLIMITED_DEPTH;
};

struct StatusOptions {
    RepositorySelector selector;
    bool modified_only = false;
    int jobs = 0;
    bool quiet = false;
};

struct UpdateOptions {
    RepositorySelector selector;
    int jobs = 0;
    int timeout_seconds = DEFAULT_TIMEOUT;
    bool quiet = false;
};

struct ExecutionOptions {
    RepositorySelector selector;
    std::string command;
    int jobs = 0;
    bool quiet = false;
};

struct RemoteSyncOptions {
    RepositorySelector selector;
    std::string source_remote;
    std::string target_remote;
    std::vector<std::string> branches;
    bool dry_run = false;
    int jobs = 0;
    int timeout_seconds = DEFAULT_TIMEOUT;
    bool quiet = false;
};

struct SyncOptions {
    std::string config_file;
    std::vector<std::string> root_patterns;
    bool prune_remotes = false;
    bool prune_branches = false;
    bool prune_repos = false;
    int jobs = 0;
    int timeout_seconds = DEFAULT_TIMEOUT;
    bool quiet = false;
};

struct ListOptions {
    RepositorySelector selector;
    bool summary_mode = false;
    bool quiet = false;
};

struct InitOptions {
    std::string repos_path;
};

#endif // SRC_LIB_UTIL_COMMAND_OPTIONS_HPP_
