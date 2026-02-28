#ifndef SRC_LIB_RUNTIME_HPP_
#define SRC_LIB_RUNTIME_HPP_

/**
 * @file runtime.hpp
 * @brief Runtime output mode detection.
 */

#include <cstdint>

/** Output renderer mode for long-running commands. */
enum class OutputMode : std::uint8_t { TUI, TEXT };

/**
 * @brief Maps terminal availability to output policy.
 *
 * @param terminal true when stdout is a terminal.
 * @return OutputMode::TUI when terminal, otherwise OutputMode::TEXT.
 */
OutputMode output_mode_from_terminal(bool terminal);

/**
 * @brief Detects whether stdout is attached to a terminal.
 */
bool is_terminal();

/**
 * @brief Detects output mode from runtime terminal state.
 */
OutputMode detect_output_mode();

#endif // SRC_LIB_RUNTIME_HPP_
