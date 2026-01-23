#include "cli.hpp"
#include "git_guard.hpp"

int main(int argc, char **argv) {
    const GitGuard git_guard;
    const int result = parse_cli(argc, argv);
    return result;
}
