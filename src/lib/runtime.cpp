#include "runtime.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

std::optional<std::string> get_env(const char *name) {
    const char *value = std::getenv(name); // NOLINT(concurrency-mt-unsafe)
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
}

bool is_terminal() {
    const auto stream_is_terminal = [](FILE *stream) {
        return ISATTY(FILENO(stream)) != 0;
    };
    const auto term_supports_tui = [] {
        const auto term = get_env("TERM");
        if (!term) {
            return false;
        }
        return *term != "dumb";
    };
    static const bool cached =
        stream_is_terminal(stdin) && stream_is_terminal(stdout) &&
        stream_is_terminal(stderr) && term_supports_tui();
    return cached;
}

OutputMode output_mode_from_terminal(bool terminal) {
    return terminal ? OutputMode::TUI : OutputMode::TEXT;
}

OutputMode detect_output_mode() {
    return output_mode_from_terminal(is_terminal());
}
