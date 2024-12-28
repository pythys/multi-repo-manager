#include <memory>
#include "git_manager.hpp"
#include "repo_factory.hpp"
#include "repo_manager.hpp"
#include "tree.hpp"

std::unique_ptr<RepoManager> create_repo_manager(RepoType type) {
    switch (type) {
        case RepoType::GIT:
            return std::make_unique<GitManager>();
        case RepoType::SVN:
            throw std::runtime_error("SVN is not supported yet");
        default:
            return nullptr;
    }
}
