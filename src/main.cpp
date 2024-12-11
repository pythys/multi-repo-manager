#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <fcntl.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

#include <cstdio>
#include "cli.hpp"
#include "git_guard.hpp"

int main(int argc, char **argv) {
    GitGuard git_guard;
    bool is_terminal = ISATTY(FILENO(stdout));
    int result = parse_cli(argc, argv, is_terminal);
    return result;
}
