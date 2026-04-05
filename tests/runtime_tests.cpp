#include "util/runtime.hpp"
#include <gtest/gtest.h>

TEST(RuntimeTests, DetectOutputModeReturnsTUIOrText) {
    const auto mode = detect_output_mode();
    EXPECT_TRUE(mode == OutputMode::TUI || mode == OutputMode::TEXT);
}

TEST(RuntimeTests, GetHomeDirectoryReturnsPathOrNullopt) {
    const auto home = get_home_directory();
    if (home) {
        EXPECT_FALSE(home->empty());
    }
}
