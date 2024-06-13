#include "cli.hpp"
#include "git_guard.hpp"

int main(int argc, char **argv) {
    GitGuard git_guard;
    int result = parse_cli(argc, argv);
    return result;
}
