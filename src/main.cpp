#include "cli.hpp"
#include "vcs/git_guard.hpp"

int main(int argc, char **argv) {
    const GitGuard git_guard;
    const int result = parse_cli(argc, argv);
    return result;
}
