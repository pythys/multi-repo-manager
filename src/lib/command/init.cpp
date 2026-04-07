#include "command/init.hpp"
#include "vcs/git_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

constexpr std::string_view kReadmeTemplate = R"(# mrm workspace

This is an mrm (multi-repo-manager) workspace for managing multiple repositories
as one using a YAML configuration file.

## directory structure

```
.
├── README.md         # this file
├── mrm.yml           # repository configuration
└── {repos_path}/     # your repositories go here
```

## quick start

### 1. clone repositories

```sh
git clone <url> {repos_path}/<name>
git clone <url> {repos_path}/<name>
```

### 2. discover and track

```sh
mrm find {repos_path} --save
```

### 3. commit configuration

```sh
git add mrm.yml
git commit -m "Add repos"
```

## common commands

List all repositories:
```sh
mrm list
```

Check status across all repos:
```sh
mrm status
```

Update all repositories:
```sh
mrm update
```

Sync repositories from configuration:
```sh
mrm sync
```

## documentation

- [Usage Guide](https://git.pythys.com/taher/multi-repo-manager/src/branch/master/docs/usage.md)
- [Quick Start](https://git.pythys.com/taher/multi-repo-manager/src/branch/master/docs/guides/quickstart.md)
- [Best Practices](https://git.pythys.com/taher/multi-repo-manager/src/branch/master/docs/guides/best-practices.md)
- [YAML Schema](https://git.pythys.com/taher/multi-repo-manager/src/branch/master/docs/yaml-schema.md)

For more information, visit: https://git.pythys.com/taher/multi-repo-manager
)";

constexpr std::string_view kConfigTemplate = R"(# This is your mrm config file
# Use 'mrm find <path> --save' to populate it with your repositories
# Example: mrm find {repos_path} --save
# See: https://git.pythys.com/taher/multi-repo-manager for documentation
trees: []
)";

std::string
substitute_repos_path(std::string_view tmpl, const std::string &repos_path) {
    std::string result(tmpl);
    const std::string placeholder = "{repos_path}";
    std::size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
        result.replace(pos, placeholder.length(), repos_path);
        pos += repos_path.length();
    }
    return result;
}

void create_gitignore_with_repos(const std::string &repos_path) {
    std::ofstream file(".gitignore");
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create .gitignore");
    }

    file << "# mrm repositories\n" << repos_path << "/\n";
    if (file.fail()) {
        throw std::runtime_error("Failed to write to .gitignore");
    }
    file.close();
}

} // namespace

int run_init(const InitOptions &options) {
    try {
        const std::string &repos_path = options.repos_path;

        std::error_code ec;
        if (!fs::is_empty(".", ec)) {
            if (ec) {
                std::cerr << "Error: Failed to check directory: "
                          << ec.message() << '\n';
            } else {
                std::cerr << "Error: Directory is not empty\n";
                std::cerr << "The init command requires a completely empty "
                             "directory.\n";
            }
            return 1;
        }

        if (!fs::create_directories(repos_path, ec) || ec) {
            std::cerr << "Error: Failed to create directory " << repos_path
                      << '\n';
            return 1;
        }

        const std::string readme_content =
            substitute_repos_path(kReadmeTemplate, repos_path);
        std::ofstream readme_file("README.md");
        if (!readme_file.is_open()) {
            std::cerr << "Error: Failed to create README.md\n";
            return 1;
        }
        readme_file << readme_content;
        if (readme_file.fail()) {
            std::cerr << "Error: Failed to write to README.md\n";
            return 1;
        }
        readme_file.close();

        const std::string config_content =
            substitute_repos_path(kConfigTemplate, repos_path);
        std::ofstream config_file("mrm.yml");
        if (!config_file.is_open()) {
            std::cerr << "Error: Failed to create mrm.yml\n";
            return 1;
        }
        config_file << config_content;
        if (config_file.fail()) {
            std::cerr << "Error: Failed to write to mrm.yml\n";
            return 1;
        }
        config_file.close();

        create_gitignore_with_repos(repos_path);

        GitManager::init(".", "master");

        const fs::path current_path = fs::current_path(ec);
        const std::string display_path = ec ? "." : current_path.string();

        std::cout << "Initialized mrm workspace in " << display_path << "\n\n";
        std::cout << "Created:\n";
        std::cout << "  README.md\n";
        std::cout << "  mrm.yml\n";
        std::cout << "  " << repos_path << "/\n";
        std::cout << "  .gitignore\n\n";
        std::cout << "Next steps:\n";
        std::cout << "  1. Clone repositories into " << repos_path << "/:\n";
        std::cout << "     git clone <url> " << repos_path << "/<name>\n\n";
        std::cout << "  2. Discover repositories:\n";
        std::cout << "     mrm find " << repos_path << " --save\n\n";
        std::cout << "  3. Commit the configuration:\n";
        std::cout << "     git add mrm.yml\n";
        std::cout << "     git commit -m \"Add initial repos\"\n\n";
        std::cout << "  4. Start managing repositories:\n";
        std::cout << "     mrm list\n";
        std::cout << "     mrm status\n";
        std::cout << "     mrm update\n\n";
        std::cout << "See README.md for more details.\n";

        return 0;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
