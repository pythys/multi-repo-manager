#include "repo_factory.hpp"
#include "git_manager.hpp"
#include "repo_manager.hpp"
#include "tree.hpp"
#include <memory>

std::unique_ptr<RepoManager> create_repo_manager(RepoType type) {
    switch (type) {
    case RepoType::GIT:
        return std::make_unique<GitManager>();
    case RepoType::SVN:
        throw std::runtime_error("Subversion is not supported yet");
    case RepoType::HG:
        throw std::runtime_error("Mercurial is not supported yet");
    default:
        return nullptr;
    }
}
