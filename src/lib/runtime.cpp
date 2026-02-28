#include "runtime.hpp"
#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

bool is_terminal() {
    static const bool cached = ISATTY(FILENO(stdout));
    return cached;
}

OutputMode output_mode_from_terminal(bool terminal) {
    return terminal ? OutputMode::TUI : OutputMode::TEXT;
}

OutputMode detect_output_mode() {
    return output_mode_from_terminal(is_terminal());
}
