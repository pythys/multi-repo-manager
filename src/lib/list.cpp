#include "list.hpp"
#include "config.hpp"
#include "output_view.hpp"
#include "tracker.hpp"
#include "tree.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool all_repos_empty(const std::vector<Tree> &trees) {
    return std::ranges::all_of(trees, [](const Tree &t) {
        return t.repos.empty();
    });
}
} // namespace

int run_list(const ListOptions &options) {
    const auto config = load_trees(
        options.selector.config_file,
        options.selector.find_paths,
        options.selector.root_patterns,
        options.selector.name_patterns);

    if (config.empty() || all_repos_empty(config)) {
        if (options.summary_mode) {
            std::cout << "No trees found.\n";
        } else {
            std::cout << "No repositories found.\n";
        }
        return 0;
    }

    const DisplayFormat format =
        options.summary_mode ? DisplayFormat::SUMMARY : DisplayFormat::TABLE;

    TrackedOperation op(config, format);
    auto &tracker = op.tracker();

    for (const auto &tree : config) {
        for (const auto &repo : tree.repos) {
            tracker.set_phase(tree.root, repo.name, RepoPhase::SUCCEEDED, "");
        }
    }

    return 0;
}
