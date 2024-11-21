#include <memory>
#include "git_manager.hpp"
#include "repo_factory.hpp"

std::unique_ptr<RepoManager> create_repo_manager(RepoType type) {
    switch (type) {
        case RepoType::GIT:
            return std::make_unique<GitManager>();
        case RepoType::SVN:
            return nullptr;
        default:
            return nullptr;
    }
}
