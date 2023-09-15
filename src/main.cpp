#include <CLI/CLI.hpp>
#include <iostream>

int main(int argc, char **argv) {
    CLI::App app("multi-repo-manager");
    std::string name;
    app.add_option("--name", name, "Your name");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if(!name.empty()) {
        std::cout << "Hello, " << name << "!\n";
    } else {
        std::cout << "Hello, World!\n";
    }

    return 0;
}

