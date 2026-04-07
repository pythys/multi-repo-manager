#ifndef SRC_LIB_CORE_TREE_HPP_
#define SRC_LIB_CORE_TREE_HPP_

#include <cstdint>
#include <string>
#include <vector>

enum class RepoPhase : std::uint8_t { QUEUED, RUNNING, SUCCEEDED, FAILED };
enum class MessageLevel : std::uint8_t { INFO, WARNING, ERROR, OUTPUT };

struct Message {
    std::string text;
    MessageLevel level;
};

struct Remote {
    std::string name;
    std::string url;
};

struct Branch {
    std::string name;
    std::string remote;
    bool is_current;
};

struct Repo {
    std::string name;
    RepoPhase phase = RepoPhase::QUEUED;
    std::vector<Remote> remotes;
    std::vector<Branch> branches;
    std::vector<Repo> children;
    std::vector<Message> messages;
};

struct Tree {
    std::string root;
    std::vector<Repo> repos;
};

struct RepoStatus {
    std::vector<std::string> messages;
    bool has_changes = false;
};

const Branch *find_current_branch(const std::vector<Branch> &branches);

#endif // SRC_LIB_CORE_TREE_HPP_
