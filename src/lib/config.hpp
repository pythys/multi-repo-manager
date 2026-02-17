#ifndef SRC_LIB_CONFIG_HPP_
#define SRC_LIB_CONFIG_HPP_

/**
 * @file config.hpp
 * @brief Configuration file handling utilities.
 *
 * Provides functions to serialize and deserialize repository
 * tree information to and from a configuration file.
 */

#include "tree.hpp"
#include <string>
#include <vector>

/**
 * @brief Writes repository configuration to a file.
 *
 * Serializes the provided tree structures and writes them
 * to the specified configuration file.
 *
 * @param trees Collection of repository trees to persist.
 * @param config_file Path to the configuration file.
 */
void write_config(
    const std::vector<Tree> &trees,
    const std::string &config_file);
/**
 * @brief Generates a configuration string from repository trees.
 *
 * Serializes the provided tree structures into a string.
 *
 * @param trees Collection of repository trees to serialize.
 *
 * @return A string containing the serialized configuration.
 */
std::string make_config(const std::vector<Tree> &trees);
/**
 * @brief Loads repository configuration from a file.
 *
 * Parses the specified configuration file and reconstructs
 * the stored repository trees.
 *
 * @param config_file Path to the configuration file.
 *
 * @return A collection of repository trees defined in the file.
 */
std::vector<Tree> get_config(const std::string &config_file);

/**
 * @brief Loads repositories with child repos from a file.
 *
 * Parses the configuration file and returns top level repositories
 * and their children (nested path)
 *
 * @param config_file Path to the configuration file.
 *
 * @return A collection of repository trees with dependencies
 */
std::vector<Tree> get_dependencies(const std::string &config_file);

#endif // SRC_LIB_CONFIG_HPP_
