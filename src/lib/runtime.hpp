#ifndef SRC_LIB_RUNTIME_HPP_
#define SRC_LIB_RUNTIME_HPP_

#include <cstdint>
#include <optional>
#include <string>

enum class OutputMode : std::uint8_t { TUI, TEXT };

OutputMode output_mode_from_terminal(bool terminal);

std::optional<std::string> get_env(const char *name);

bool is_terminal();

OutputMode detect_output_mode();

#endif // SRC_LIB_RUNTIME_HPP_
