#include "cli.hpp"
#include "vcs/git_guard.hpp"
#include <exception>
#include <iostream>

int main(int argc, char **argv) {
    const GitGuard git_guard;
    try {
        return parse_cli(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
