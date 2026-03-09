#ifndef SRC_LIB_RUNTIME_HPP_
#define SRC_LIB_RUNTIME_HPP_

#include <cstdint>

enum class OutputMode : std::uint8_t { TUI, TEXT };

OutputMode output_mode_from_terminal(bool terminal);

bool is_terminal();

OutputMode detect_output_mode();

#endif // SRC_LIB_RUNTIME_HPP_
