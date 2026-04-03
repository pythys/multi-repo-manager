#ifndef SRC_LIB_RUNTIME_HPP_
#define SRC_LIB_RUNTIME_HPP_

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

enum class OutputMode : std::uint8_t { TUI, TEXT };

std::optional<std::string> get_env(const char *name);
std::optional<std::filesystem::path> get_home_directory();

OutputMode detect_output_mode();

#endif // SRC_LIB_RUNTIME_HPP_
