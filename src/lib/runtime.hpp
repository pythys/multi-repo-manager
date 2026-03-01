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
 * @brief Maps interactive terminal availability to output policy.
 *
 * @param terminal true when interactive terminal features are available.
 * @return OutputMode::TUI when terminal, otherwise OutputMode::TEXT.
 */
OutputMode output_mode_from_terminal(bool terminal);

/**
 * @brief Detects whether interactive terminal features are available.
 *
 * Returns true only when stdin, stdout, and stderr are terminals and TERM is
 * suitable for interactive rendering.
 */
bool is_terminal();

/**
 * @brief Detects output mode from runtime terminal state.
 */
OutputMode detect_output_mode();

#endif // SRC_LIB_RUNTIME_HPP_
