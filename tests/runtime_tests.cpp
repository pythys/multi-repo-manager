#include "runtime.hpp"
#include <gtest/gtest.h>

TEST(RuntimeTests, OutputModeFromTerminalTrue) {
    EXPECT_EQ(OutputMode::TUI, output_mode_from_terminal(true));
}

TEST(RuntimeTests, OutputModeFromTerminalFalse) {
    EXPECT_EQ(OutputMode::TEXT, output_mode_from_terminal(false));
}

TEST(RuntimeTests, DetectOutputModeMatchesTerminalState) {
    EXPECT_EQ(output_mode_from_terminal(is_terminal()), detect_output_mode());
}
