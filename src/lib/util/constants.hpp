#ifndef SRC_LIB_UTIL_CONSTANTS_HPP_
#define SRC_LIB_UTIL_CONSTANTS_HPP_

#include <limits>
#include <string_view>

inline constexpr int SYNC_POOL_SIZE = 10;
inline constexpr int DEFAULT_TIMEOUT = 300;
inline constexpr int UNLIMITED_DEPTH = std::numeric_limits<int>::max();

#endif // SRC_LIB_UTIL_CONSTANTS_HPP_
