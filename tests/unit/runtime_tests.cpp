#include "util/runtime.hpp"
#include <gtest/gtest.h>

TEST(RuntimeTests, GetHomeDirectoryReturnsPathOrNullopt) {
    const auto home = get_home_directory();
    if (home) {
        EXPECT_FALSE(home->empty());
    }
}
