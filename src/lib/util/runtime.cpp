#include "util/runtime.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

std::optional<std::filesystem::path> get_home_directory() {
#if defined(_WIN32) || defined(_WIN64)
    const auto userprofile = get_env("USERPROFILE");
    if (userprofile) {
        return std::filesystem::path(*userprofile);
    }
    const auto homedrive = get_env("HOMEDRIVE");
    const auto homepath = get_env("HOMEPATH");
    if (homedrive && homepath) {
        return std::filesystem::path(*homedrive) / *homepath;
    }
    return std::nullopt;
#else
    const auto home = get_env("HOME");
    if (home) {
        return std::filesystem::path(*home);
    }
    return std::nullopt;
#endif
}

OutputMode detect_output_mode() {
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
    return cached ? OutputMode::TUI : OutputMode::TEXT;
}
