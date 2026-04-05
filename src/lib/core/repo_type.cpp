#include "core/repo_type.hpp"
#include <optional>
#include <stdexcept>
#include <string_view>

std::optional<RepoType> parse_repo_type(std::string_view text) {
    if (text == "git") {
        return RepoType::GIT;
    }
    if (text == "svn") {
        return RepoType::SVN;
    }
    if (text == "hg") {
        return RepoType::HG;
    }
    return std::nullopt;
}

std::string_view repo_type_name(RepoType type) {
    switch (type) {
    case RepoType::GIT:
        return "git";
    case RepoType::SVN:
        return "svn";
    case RepoType::HG:
        return "hg";
    default:
        throw std::runtime_error("Unknown RepoType");
    }
}
