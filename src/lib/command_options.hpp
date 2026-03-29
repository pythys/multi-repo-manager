#ifndef SRC_LIB_COMMAND_OPTIONS_HPP_
#define SRC_LIB_COMMAND_OPTIONS_HPP_

#include <string>
#include <vector>

struct RepositorySelector {
    std::string config_file;
    std::vector<std::string> find_paths;
    std::vector<std::string> root_patterns;
    std::vector<std::string> name_patterns;
};

struct StatusOptions {
    RepositorySelector selector;
    bool modified_only = false;
};

struct UpdateOptions {
    RepositorySelector selector;
    int jobs = 0;
};

struct ExecutionOptions {
    RepositorySelector selector;
    std::string command;
    std::string repository_type;
};

struct RemoteSyncOptions {
    RepositorySelector selector;
    std::string source_remote;
    std::string target_remote;
    std::vector<std::string> branches;
    bool dry_run = false;
    int jobs = 0;
};

struct SyncOptions {
    std::string config_file;
    std::vector<std::string> root_patterns;
    bool prune_remotes = false;
    bool prune_branches = false;
    int jobs = 0;
};

struct ListOptions {
    RepositorySelector selector;
    bool summary_mode = false;
};

struct InitOptions {
    std::string repos_path;
};

#endif // SRC_LIB_COMMAND_OPTIONS_HPP_
