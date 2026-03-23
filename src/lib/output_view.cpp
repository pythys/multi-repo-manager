#include "output_view.hpp"
#include "tree.hpp"
#include <algorithm>
#include <atomic>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr std::size_t kScrollSingleStep = 1;
constexpr std::size_t kScrollWheelStep = 3;

std::string convert_phase_to_string(RepoPhase phase) {
    switch (phase) {
    case RepoPhase::QUEUED:
        return "QUEUED";
    case RepoPhase::RUNNING:
        return "RUNNING";
    case RepoPhase::SUCCEEDED:
        return "SUCCEEDED";
    case RepoPhase::FAILED:
        return "FAILED";
    }
    return "UNKNOWN";
}

ftxui::Color convert_phase_to_color(RepoPhase phase) {
    switch (phase) {
    case RepoPhase::QUEUED:
        return ftxui::Color::GrayLight;
    case RepoPhase::RUNNING:
        return ftxui::Color::Yellow;
    case RepoPhase::SUCCEEDED:
        return ftxui::Color::Green;
    case RepoPhase::FAILED:
        return ftxui::Color::Red;
    }
    return ftxui::Color::White;
}

ftxui::Color convert_type_to_color(RepoType type) {
    switch (type) {
    case RepoType::GIT:
        return ftxui::Color::Green;
    case RepoType::SVN:
        return ftxui::Color::Blue;
    case RepoType::HG:
        return ftxui::Color::Magenta;
    default:
        return ftxui::Color::White;
    }
}

std::string extract_current_branch(const std::vector<Branch> &branches) {
    for (const auto &branch : branches) {
        if (branch.is_current) {
            return branch.name;
        }
    }
    return "";
}

std::string apply_padding(
    const std::string &str,
    std::size_t width,
    bool align_right = false) {
    if (str.length() >= width) {
        return str;
    }
    const std::size_t padding = width - str.length();
    if (align_right) {
        return std::string(padding, ' ') + str;
    }
    return str + std::string(padding, ' ');
}

std::string create_separator(std::size_t width) {
    return std::string(width, '-');
}

void build_progress_tui_lines(
    const std::string &root,
    const std::vector<Repo> &repos,
    std::vector<ftxui::Element> &lines) {
    using ftxui::color;
    using ftxui::hbox;
    using ftxui::separator;
    using ftxui::text;

    for (const auto &repo : repos) {
        const std::string path =
            (std::filesystem::path(root) / repo.name).string();
        const std::string phase = convert_phase_to_string(repo.phase);
        lines.push_back(hbox({
            text(phase) | color(convert_phase_to_color(repo.phase)),
            separator(),
            text(path),
        }));
        for (const auto &message : repo.messages) {
            lines.push_back(
                text("  " + message) | color(ftxui::Color::GrayLight));
        }
        build_progress_tui_lines(root, repo.children, lines);
    }
}

void build_progress_text_lines(
    const std::string &root,
    const std::vector<Repo> &repos,
    std::vector<std::string> &lines) {
    for (const auto &repo : repos) {
        const std::string path =
            (std::filesystem::path(root) / repo.name).string();
        lines.push_back(
            "[" + convert_phase_to_string(repo.phase) + "] " + path);
        for (const auto &message : repo.messages) {
            lines.push_back("  - " + message);
        }
        build_progress_text_lines(root, repo.children, lines);
    }
}

struct TableColumnWidths {
    std::size_t root;
    std::size_t name;
    std::size_t type;
    std::size_t remotes;
    std::size_t branches;
    std::size_t current;
};

TableColumnWidths compute_table_column_widths(const std::vector<Tree> &trees) {
    constexpr std::size_t kMinRootWidth = 4;
    constexpr std::size_t kMinNameWidth = 4;
    constexpr std::size_t kMinTypeWidth = 4;
    constexpr std::size_t kMinRemotesWidth = 7;
    constexpr std::size_t kMinBranchesWidth = 8;
    constexpr std::size_t kMinCurrentWidth = 7;

    TableColumnWidths widths{
        .root = kMinRootWidth,
        .name = kMinNameWidth,
        .type = kMinTypeWidth,
        .remotes = kMinRemotesWidth,
        .branches = kMinBranchesWidth,
        .current = kMinCurrentWidth};

    for (const auto &tree : trees) {
        widths.root = std::max(widths.root, tree.root.length());
        for (const auto &repo : tree.repos) {
            widths.name = std::max(widths.name, repo.name.length());
            const std::string type_str = repo_type_to_string(repo.type);
            widths.type = std::max(widths.type, type_str.length());
            const std::string remotes_str = std::to_string(repo.remotes.size());
            widths.remotes = std::max(widths.remotes, remotes_str.length());
            const std::string branches_str =
                std::to_string(repo.branches.size());
            widths.branches = std::max(widths.branches, branches_str.length());
            const std::string current = extract_current_branch(repo.branches);
            widths.current = std::max(widths.current, current.length());
        }
    }
    return widths;
}

void build_table_tui_lines(
    const std::vector<Tree> &trees,
    std::vector<ftxui::Element> &lines) {
    using namespace ftxui;

    if (trees.empty()) {
        return;
    }

    const TableColumnWidths widths = compute_table_column_widths(trees);

    lines.push_back(
        hbox({
            text(apply_padding("ROOT", widths.root)) | bold,
            text("  "),
            text(apply_padding("NAME", widths.name)) | bold,
            text("  "),
            text(apply_padding("TYPE", widths.type)) | bold,
            text("  "),
            text(apply_padding("REMOTES", widths.remotes, true)) | bold,
            text("  "),
            text(apply_padding("BRANCHES", widths.branches, true)) | bold,
            text("  "),
            text(apply_padding("CURRENT", widths.current)) | bold,
        }) |
        color(Color::White));

    constexpr std::size_t kColumnSpacing = 10;
    const std::size_t separator_width =
        widths.root + widths.name + widths.type + widths.remotes +
        widths.branches + widths.current + kColumnSpacing;
    lines.push_back(text(create_separator(separator_width)));

    for (const auto &tree : trees) {
        for (const auto &repo : tree.repos) {
            const std::string type_str = repo_type_to_string(repo.type);
            const std::string remotes_str = std::to_string(repo.remotes.size());
            const std::string branches_str =
                std::to_string(repo.branches.size());
            const std::string current = extract_current_branch(repo.branches);

            lines.push_back(hbox({
                text(apply_padding(tree.root, widths.root)) |
                    color(Color::Cyan),
                text("  "),
                text(apply_padding(repo.name, widths.name)),
                text("  "),
                text(apply_padding(type_str, widths.type)) |
                    color(convert_type_to_color(repo.type)),
                text("  "),
                text(apply_padding(remotes_str, widths.remotes, true)),
                text("  "),
                text(apply_padding(branches_str, widths.branches, true)),
                text("  "),
                text(apply_padding(current, widths.current)) |
                    color(Color::Yellow),
            }));
        }
    }
}

std::string format_table_text_row(
    const std::vector<std::string> &cells,
    const TableColumnWidths &widths) {
    constexpr std::size_t kExpectedColumnCount = 6;
    constexpr std::size_t kRootIndex = 0;
    constexpr std::size_t kNameIndex = 1;
    constexpr std::size_t kTypeIndex = 2;
    constexpr std::size_t kRemotesIndex = 3;
    constexpr std::size_t kBranchesIndex = 4;
    constexpr std::size_t kCurrentIndex = 5;

    std::ostringstream oss;
    if (cells.size() >= kExpectedColumnCount) {
        oss << apply_padding(cells[kRootIndex], widths.root) << "  "
            << apply_padding(cells[kNameIndex], widths.name) << "  "
            << apply_padding(cells[kTypeIndex], widths.type) << "  "
            << apply_padding(cells[kRemotesIndex], widths.remotes, true) << "  "
            << apply_padding(cells[kBranchesIndex], widths.branches, true)
            << "  " << apply_padding(cells[kCurrentIndex], widths.current);
    }
    return oss.str();
}

void build_table_text_lines(
    const std::vector<Tree> &trees,
    std::vector<std::string> &lines) {
    if (trees.empty()) {
        return;
    }

    const TableColumnWidths widths = compute_table_column_widths(trees);

    lines.push_back(format_table_text_row(
        {"ROOT", "NAME", "TYPE", "REMOTES", "BRANCHES", "CURRENT"},
        widths));

    constexpr std::size_t kColumnSpacing = 10;
    const std::size_t separator_width =
        widths.root + widths.name + widths.type + widths.remotes +
        widths.branches + widths.current + kColumnSpacing;
    lines.push_back(create_separator(separator_width));

    for (const auto &tree : trees) {
        for (const auto &repo : tree.repos) {
            const std::string type_str = repo_type_to_string(repo.type);
            const std::string current = extract_current_branch(repo.branches);

            lines.push_back(format_table_text_row(
                {tree.root,
                 repo.name,
                 type_str,
                 std::to_string(repo.remotes.size()),
                 std::to_string(repo.branches.size()),
                 current},
                widths));
        }
    }
}

struct SummaryColumnWidths {
    std::size_t root;
    std::size_t repos;
};

SummaryColumnWidths
compute_summary_column_widths(const std::vector<Tree> &trees) {
    constexpr std::size_t kMinRootWidth = 4;
    constexpr std::size_t kMinReposWidth = 5;

    SummaryColumnWidths widths{.root = kMinRootWidth, .repos = kMinReposWidth};

    for (const auto &tree : trees) {
        widths.root = std::max(widths.root, tree.root.length());
        const std::string repos_str = std::to_string(tree.repos.size());
        widths.repos = std::max(widths.repos, repos_str.length());
    }
    return widths;
}

void build_summary_tui_lines(
    const std::vector<Tree> &trees,
    std::vector<ftxui::Element> &lines) {
    using namespace ftxui;

    if (trees.empty()) {
        return;
    }

    const SummaryColumnWidths widths = compute_summary_column_widths(trees);

    lines.push_back(
        hbox({
            text(apply_padding("ROOT", widths.root)) | bold,
            text("  "),
            text(apply_padding("REPOS", widths.repos, true)) | bold,
        }) |
        color(Color::White));

    constexpr std::size_t kColumnSpacing = 2;
    const std::size_t separator_width =
        widths.root + widths.repos + kColumnSpacing;
    lines.push_back(text(create_separator(separator_width)));

    for (const auto &tree : trees) {
        const std::string repos_str = std::to_string(tree.repos.size());
        lines.push_back(hbox({
            text(apply_padding(tree.root, widths.root)) | color(Color::Cyan),
            text("  "),
            text(apply_padding(repos_str, widths.repos, true)),
        }));
    }
}

void build_summary_text_lines(
    const std::vector<Tree> &trees,
    std::vector<std::string> &lines) {
    if (trees.empty()) {
        return;
    }

    const SummaryColumnWidths widths = compute_summary_column_widths(trees);

    std::ostringstream header;
    header << apply_padding("ROOT", widths.root) << "  "
           << apply_padding("REPOS", widths.repos, true);
    lines.push_back(header.str());

    constexpr std::size_t kColumnSpacing = 2;
    const std::size_t separator_width =
        widths.root + widths.repos + kColumnSpacing;
    lines.push_back(create_separator(separator_width));

    for (const auto &tree : trees) {
        std::ostringstream row;
        row << apply_padding(tree.root, widths.root) << "  "
            << apply_padding(
                   std::to_string(tree.repos.size()),
                   widths.repos,
                   true);
        lines.push_back(row.str());
    }
}

std::vector<ftxui::Element>
build_tui_output(const std::vector<Tree> &trees, DisplayFormat format) {
    using ftxui::bold;
    using ftxui::hbox;
    using ftxui::separator;
    using ftxui::text;

    std::vector<ftxui::Element> lines;

    if (format == DisplayFormat::TABLE) {
        lines.push_back(hbox({text("mrm"), separator(), text("list")}) | bold);
        lines.push_back(ftxui::separator());
        build_table_tui_lines(trees, lines);
        return lines;
    }

    if (format == DisplayFormat::SUMMARY) {
        lines.push_back(
            hbox({text("mrm"), separator(), text("list summary")}) | bold);
        lines.push_back(ftxui::separator());
        build_summary_tui_lines(trees, lines);
        return lines;
    }

    lines.push_back(
        hbox({text("mrm"), separator(), text("live status")}) | bold);
    lines.push_back(ftxui::separator());
    for (const auto &tree : trees) {
        lines.push_back(text(tree.root) | bold);
        build_progress_tui_lines(tree.root, tree.repos, lines);
        lines.push_back(text(""));
    }
    return lines;
}

void build_text_output(
    const std::vector<Tree> &trees,
    DisplayFormat format,
    std::vector<std::string> &lines) {

    if (format == DisplayFormat::TABLE) {
        lines.emplace_back("mrm list");
        build_table_text_lines(trees, lines);
        return;
    }

    if (format == DisplayFormat::SUMMARY) {
        lines.emplace_back("mrm list");
        build_summary_text_lines(trees, lines);
        return;
    }

    lines.emplace_back("mrm report");
    for (const auto &tree : trees) {
        lines.emplace_back("");
        lines.emplace_back(tree.root);
        build_progress_text_lines(tree.root, tree.repos, lines);
    }
}

void render_text_output(const Tracker &tracker, DisplayFormat format) {
    const std::vector<Tree> trees = tracker.snapshot();
    std::vector<std::string> lines;
    build_text_output(trees, format, lines);

    for (const auto &line : lines) {
        std::cout << line << '\n';
    }
    std::cout.flush();
}

void render_tui_output(const Tracker &tracker, DisplayFormat format) {
    using ftxui::vbox;

    const std::vector<Tree> trees = tracker.snapshot();
    auto lines = build_tui_output(trees, format);
    const std::size_t line_count = lines.size();

    const auto size = ftxui::Terminal::Size();
    const int width = size.dimx > 0 ? size.dimx : 80;
    const int height = line_count > 0 ? static_cast<int>(line_count) : 1;

    auto screen = ftxui::Screen::Create(
        ftxui::Dimension::Fixed(width),
        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, vbox(std::move(lines)));
    std::cout << screen.ToString();
    std::cout.flush();
}

void reset_terminal_state() {
    std::cout << "\x1b[0 q"
              << "\x1b[?1000l"
              << "\x1b[?1002l"
              << "\x1b[?1003l"
              << "\x1b[?1006l"
              << "\x1b[?1015l";
    std::cout.flush();
}

class TextView final : public OutputView {
  public:
    explicit TextView(Tracker &tracker, DisplayFormat format)
        : tracker_(tracker), format_(format) {}

    ~TextView() override = default;

    void start() override {}

    void stop() override {
        if (stopped_.exchange(true)) {
            return;
        }
        render_text_output(tracker_, format_);
    }

  private:
    Tracker &tracker_;
    DisplayFormat format_;
    std::atomic<bool> stopped_ = false;
};

class TuiView final : public OutputView {
  public:
    explicit TuiView(Tracker &tracker, DisplayFormat format)
        : tracker_(tracker), format_(format) {}

    ~TuiView() override {
        stop();
    }

    void start() override {
        if (started_.exchange(true)) {
            return;
        }
        {
            std::scoped_lock<std::mutex> lock(data_mutex_);
            trees_ = tracker_.snapshot();
        }
        ui_thread_ = std::thread([this] { run_ui_loop(); });
        event_thread_ = std::thread([this] { process_tracker_events(); });
    }

    void stop() override {
        if (!started_.exchange(false)) {
            return;
        }
        tracker_.close();

        if (event_thread_.joinable()) {
            event_thread_.join();
        }

        std::function<void()> exit_handler;
        {
            std::scoped_lock<std::mutex> lock(screen_mutex_);
            exit_handler = exit_loop_;
        }
        if (exit_handler) {
            exit_handler();
        }

        if (ui_thread_.joinable()) {
            ui_thread_.join();
        }

        render_tui_output(tracker_, format_);
    }

  private:
    void scroll_up(std::size_t lines) {
        if (selected_line_ > lines) {
            selected_line_ -= lines;
            return;
        }
        selected_line_ = 0;
    }

    void scroll_down(std::size_t lines) {
        if (line_count_ == 0) {
            selected_line_ = 0;
            return;
        }
        const std::size_t max_line = line_count_ - 1;
        if (lines > max_line - selected_line_) {
            selected_line_ = max_line;
            return;
        }
        selected_line_ += lines;
    }

    bool handle_input_event(ftxui::Event event) {
        if (event == ftxui::Event::ArrowUp) {
            scroll_up(kScrollSingleStep);
            return true;
        }
        if (event == ftxui::Event::ArrowDown) {
            scroll_down(kScrollSingleStep);
            return true;
        }
        if (event.is_mouse()) {
            const auto button = event.mouse().button;
            if (button == ftxui::Mouse::WheelUp) {
                scroll_up(kScrollWheelStep);
                return true;
            }
            if (button == ftxui::Mouse::WheelDown) {
                scroll_down(kScrollWheelStep);
                return true;
            }
        }
        return false;
    }

    ftxui::Element render_ui() {
        using ftxui::flex;
        using ftxui::frame;
        using ftxui::vbox;
        using ftxui::vscroll_indicator;

        std::vector<Tree> trees;
        {
            std::scoped_lock<std::mutex> lock(data_mutex_);
            trees = trees_;
        }

        auto lines = build_tui_output(trees, format_);
        line_count_ = lines.size();

        if (line_count_ == 0) {
            selected_line_ = 0;
        } else if (selected_line_ >= line_count_) {
            selected_line_ = line_count_ - 1;
            lines.at(selected_line_) = lines.at(selected_line_) | ftxui::focus;
        } else {
            lines.at(selected_line_) = lines.at(selected_line_) | ftxui::focus;
        }

        return vbox(
            {vbox(std::move(lines)) | vscroll_indicator | frame | flex});
    }

    void process_tracker_events() {
        TrackerEvent event;
        while (tracker_.wait_next_event(event)) {
            {
                std::scoped_lock<std::mutex> lock(data_mutex_);
                trees_ = tracker_.snapshot();
            }
            std::function<void()> refresh_handler;
            {
                std::scoped_lock<std::mutex> lock(screen_mutex_);
                refresh_handler = post_refresh_;
            }
            if (refresh_handler) {
                refresh_handler();
            }
        }
    }

    void run_ui_loop() {
        auto screen = ftxui::ScreenInteractive::Fullscreen();
        {
            std::scoped_lock<std::mutex> lock(screen_mutex_);
            exit_loop_ = screen.ExitLoopClosure();
            post_refresh_ = [&screen] {
                screen.PostEvent(ftxui::Event::Custom);
            };
        }

        auto renderer = ftxui::Renderer([this] { return render_ui(); });
        auto component =
            ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
                return handle_input_event(std::move(event));
            });

        screen.Loop(component);
        reset_terminal_state();

        {
            std::scoped_lock<std::mutex> lock(screen_mutex_);
            post_refresh_ = {};
            exit_loop_ = {};
        }
    }

    Tracker &tracker_;
    DisplayFormat format_;
    std::atomic<bool> started_ = false;
    std::thread ui_thread_;
    std::thread event_thread_;
    std::mutex data_mutex_;
    std::vector<Tree> trees_;
    std::mutex screen_mutex_;
    std::function<void()> post_refresh_;
    std::function<void()> exit_loop_;
    std::size_t selected_line_ = 0;
    std::size_t line_count_ = 0;
};

} // namespace

std::unique_ptr<OutputView>
create_output_view(OutputMode mode, DisplayFormat format, Tracker &tracker) {
    if (mode == OutputMode::TUI) {
        return std::make_unique<TuiView>(tracker, format);
    }
    return std::make_unique<TextView>(tracker, format);
}
