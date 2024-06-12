#include <memory>
#include "cli.hpp"
#include "git_manager.hpp"

int main(int argc, char **argv) {
    std::unique_ptr<GitManager> git_manager = std::make_unique<GitManager>();
    git_manager->init();
    int result = parse_cli(argc, argv);
    git_manager->shutdown();
    return result;
}
