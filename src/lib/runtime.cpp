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
