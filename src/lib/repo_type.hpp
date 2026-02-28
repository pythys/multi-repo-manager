#ifndef SRC_LIB_REPO_TYPE_HPP_
#define SRC_LIB_REPO_TYPE_HPP_

/**
 * @file repo_type.hpp
 * @brief String conversion helpers for RepoType.
 */

#include "tree.hpp"
#include <optional>
#include <string_view>

/**
 * @brief Parse a repository type name into RepoType.
 *
 * Accepts lower-case values used by config/CLI such as "git", "svn", and
 * "hg".
 */
std::optional<RepoType> parse_repo_type(std::string_view text);

/** @brief Return lower-case name for a RepoType. */
std::string_view repo_type_name(RepoType type);

#endif // SRC_LIB_REPO_TYPE_HPP_
