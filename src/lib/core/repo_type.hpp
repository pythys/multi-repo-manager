#ifndef SRC_LIB_CORE_REPO_TYPE_HPP_
#define SRC_LIB_CORE_REPO_TYPE_HPP_

#include "core/tree.hpp"
#include <optional>
#include <string_view>

std::optional<RepoType> parse_repo_type(std::string_view text);

std::string_view repo_type_name(RepoType type);

#endif // SRC_LIB_CORE_REPO_TYPE_HPP_
