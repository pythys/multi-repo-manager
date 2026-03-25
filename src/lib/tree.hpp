#ifndef SRC_LIB_TREE_HPP_
#define SRC_LIB_TREE_HPP_

#include <cstdint>
#include <string>
#include <vector>

enum class RepoType : std::uint8_t { GIT, SVN, HG, UNKNOWN };
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
    RepoType type = RepoType::UNKNOWN;
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

std::string repo_type_to_string(RepoType type);

const Branch *find_current_branch(const std::vector<Branch> &branches);

#endif // SRC_LIB_TREE_HPP_
