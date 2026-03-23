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
constexpr std::size_t kSingleStepLines = 1;
constexpr std::size_t kWheelStepLines = 3;

void restore_terminal_state() {
    // Reset cursor style to terminal default and disable mouse tracking modes.
    std::cout << "\x1b[0 q"
              << "\x1b[?1000l"
              << "\x1b[?1002l"
              << "\x1b[?1003l"
              << "\x1b[?1006l"
              << "\x1b[?1015l";
    std::cout.flush();
}

std::string phase_to_string(RepoPhase phase) {
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

ftxui::Color phase_to_color(RepoPhase phase) {
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

ftxui::Color type_to_color(RepoType type) {
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

std::string get_current_branch(const std::vector<Branch> &branches) {
    for (const auto &branch : branches) {
        if (branch.is_current) {
            return branch.name;
        }
    }
    return "";
}

void flatten_repo_lines(
    const std::string &root,
    const std::vector<Repo> &repos,
    std::vector<ftxui::Element> &lines) {
    using ftxui::color;
    using ftxui::hbox;
    using ftxui::separator;
    using ftxui::text;

    for (const auto &repo : repos) {
        const std::string path = root + "/" + repo.name;
        const std::string phase = phase_to_string(repo.phase);
        lines.push_back(hbox({
            text(phase) | color(phase_to_color(repo.phase)),
            separator(),
            text(path),
        }));
        for (const auto &message : repo.messages) {
            lines.push_back(
                text("  " + message) | color(ftxui::Color::GrayLight));
        }
        flatten_repo_lines(root, repo.children, lines);
    }
}

void flatten_repo_lines_text(
    const std::string &root,
    const std::vector<Repo> &repos,
    std::vector<std::string> &lines) {
    for (const auto &repo : repos) {
        const std::string path = root + "/" + repo.name;
        lines.push_back("[" + phase_to_string(repo.phase) + "] " + path);
        for (const auto &message : repo.messages) {
            lines.push_back("  - " + message);
        }
        flatten_repo_lines_text(root, repo.children, lines);
    }
}

struct ColumnWidths {
    std::size_t root;
    std::size_t name;
    std::size_t type;
    std::size_t remotes;
    std::size_t branches;
    std::size_t current;
};

ColumnWidths calculate_column_widths(const std::vector<Tree> &trees) {
    constexpr std::size_t kMinRootWidth = 4;
    constexpr std::size_t kMinNameWidth = 4;
    constexpr std::size_t kMinTypeWidth = 4;
    constexpr std::size_t kMinRemotesWidth = 7;
    constexpr std::size_t kMinBranchesWidth = 8;
    constexpr std::size_t kMinCurrentWidth = 7;
    ColumnWidths widths{
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
            const std::string current = get_current_branch(repo.branches);
            widths.current = std::max(widths.current, current.length());
        }
    }
    return widths;
}

std::string pad_string(
    const std::string &str,
    std::size_t width,
    bool right_align = false) {
    if (str.length() >= width) {
        return str;
    }
    const std::size_t padding = width - str.length();
    if (right_align) {
        return std::string(padding, ' ') + str;
    }
    return str + std::string(padding, ' ');
}

std::string format_table_row(
    const std::vector<std::string> &cells,
    const ColumnWidths &widths) {
    constexpr std::size_t kExpectedColumnCount = 6;
    constexpr std::size_t kRootIndex = 0;
    constexpr std::size_t kNameIndex = 1;
    constexpr std::size_t kTypeIndex = 2;
    constexpr std::size_t kRemotesIndex = 3;
    constexpr std::size_t kBranchesIndex = 4;
    constexpr std::size_t kCurrentIndex = 5;
    std::ostringstream oss;
    if (cells.size() >= kExpectedColumnCount) {
        oss << pad_string(cells[kRootIndex], widths.root) << "  "
            << pad_string(cells[kNameIndex], widths.name) << "  "
            << pad_string(cells[kTypeIndex], widths.type) << "  "
            << pad_string(cells[kRemotesIndex], widths.remotes, true) << "  "
            << pad_string(cells[kBranchesIndex], widths.branches, true) << "  "
            << pad_string(cells[kCurrentIndex], widths.current);
    }
    return oss.str();
}

std::string format_separator(const ColumnWidths &widths) {
    constexpr std::size_t kColumnSpacing = 10;
    const std::size_t total_width = widths.root + widths.name + widths.type +
                                    widths.remotes + widths.branches +
                                    widths.current + kColumnSpacing;
    return std::string(total_width, '-');
}

void flatten_repo_table_lines(
    const std::vector<Tree> &trees,
    std::vector<ftxui::Element> &lines,
    const ColumnWidths &widths) {
    using namespace ftxui;

    if (trees.empty()) {
        return;
    }

    lines.push_back(
        hbox({
            text(pad_string("ROOT", widths.root)) | bold,
            text("  "),
            text(pad_string("NAME", widths.name)) | bold,
            text("  "),
            text(pad_string("TYPE", widths.type)) | bold,
            text("  "),
            text(pad_string("REMOTES", widths.remotes, true)) | bold,
            text("  "),
            text(pad_string("BRANCHES", widths.branches, true)) | bold,
            text("  "),
            text(pad_string("CURRENT", widths.current)) | bold,
        }) |
        color(Color::White));
    lines.push_back(text(format_separator(widths)));

    for (const auto &tree : trees) {
        for (const auto &repo : tree.repos) {
            const std::string type_str = repo_type_to_string(repo.type);
            const std::string remotes_str = std::to_string(repo.remotes.size());
            const std::string branches_str =
                std::to_string(repo.branches.size());
            const std::string current = get_current_branch(repo.branches);

            lines.push_back(hbox({
                text(pad_string(tree.root, widths.root)) | color(Color::Cyan),
                text("  "),
                text(pad_string(repo.name, widths.name)),
                text("  "),
                text(pad_string(type_str, widths.type)) |
                    color(type_to_color(repo.type)),
                text("  "),
                text(pad_string(remotes_str, widths.remotes, true)),
                text("  "),
                text(pad_string(branches_str, widths.branches, true)),
                text("  "),
                text(pad_string(current, widths.current)) |
                    color(Color::Yellow),
            }));
        }
    }
}

void flatten_repo_table_lines_text(
    const std::vector<Tree> &trees,
    std::vector<std::string> &lines) {
    if (trees.empty()) {
        return;
    }

    const ColumnWidths widths = calculate_column_widths(trees);

    lines.push_back(format_table_row(
        {"ROOT", "NAME", "TYPE", "REMOTES", "BRANCHES", "CURRENT"},
        widths));
    lines.push_back(format_separator(widths));

    for (const auto &tree : trees) {
        for (const auto &repo : tree.repos) {
            const std::string type_str = repo_type_to_string(repo.type);
            const std::string current = get_current_branch(repo.branches);

            lines.push_back(format_table_row(
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

std::vector<ftxui::Element>
build_tui_lines(const std::vector<Tree> &trees, DisplayFormat format) {
    using ftxui::bold;
    using ftxui::hbox;
    using ftxui::separator;
    using ftxui::text;
    std::vector<ftxui::Element> lines;

    if (format == DisplayFormat::TABLE) {
        lines.push_back(hbox({text("mrm"), separator(), text("list")}) | bold);
        lines.push_back(ftxui::separator());
        const ColumnWidths widths = calculate_column_widths(trees);
        flatten_repo_table_lines(trees, lines, widths);
        return lines;
    }

    lines.push_back(
        hbox({text("mrm"), separator(), text("live status")}) | bold);
    lines.push_back(ftxui::separator());
    for (const auto &tree : trees) {
        lines.push_back(text(tree.root) | bold);
        flatten_repo_lines(tree.root, tree.repos, lines);
        lines.push_back(text(""));
    }
    return lines;
}

void print_final_report(const Tracker &tracker, DisplayFormat format) {
    const std::vector<Tree> trees = tracker.snapshot();
    std::vector<std::string> lines;

    if (format == DisplayFormat::TABLE) {
        lines.emplace_back("mrm list");
        flatten_repo_table_lines_text(trees, lines);
    } else {
        lines.emplace_back("mrm report");
        for (const auto &tree : trees) {
            lines.emplace_back("");
            lines.emplace_back(tree.root);
            flatten_repo_lines_text(tree.root, tree.repos, lines);
        }
    }

    for (const auto &line : lines) {
        std::cout << line << '\n';
    }
    std::cout.flush();
}

void print_final_tui_report(const Tracker &tracker, DisplayFormat format) {
    using ftxui::vbox;
    const std::vector<Tree> trees = tracker.snapshot();
    auto lines = build_tui_lines(trees, format);
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
        print_final_report(tracker_, format_);
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
        ui_thread_ = std::thread([this] { run_ui(); });
        event_thread_ = std::thread([this] {
            TrackerEvent event;
            while (tracker_.wait_next_event(event)) {
                {
                    std::scoped_lock<std::mutex> lock(data_mutex_);
                    trees_ = tracker_.snapshot();
                }
                std::function<void()> post;
                {
                    std::scoped_lock<std::mutex> lock(screen_mutex_);
                    post = post_refresh_;
                }
                if (post) {
                    post();
                }
            }
        });
    }

    void stop() override {
        if (!started_.exchange(false)) {
            return;
        }
        tracker_.close();
        if (event_thread_.joinable()) {
            event_thread_.join();
        }
        std::function<void()> exit;
        {
            std::scoped_lock<std::mutex> lock(screen_mutex_);
            exit = exit_loop_;
        }
        if (exit) {
            exit();
        }
        if (ui_thread_.joinable()) {
            ui_thread_.join();
        }
        print_final_tui_report(tracker_, format_);
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

    bool on_event(ftxui::Event event) {
        if (event == ftxui::Event::ArrowUp) {
            scroll_up(kSingleStepLines);
            return true;
        }
        if (event == ftxui::Event::ArrowDown) {
            scroll_down(kSingleStepLines);
            return true;
        }
        if (event.is_mouse()) {
            const auto button = event.mouse().button;
            if (button == ftxui::Mouse::WheelUp) {
                scroll_up(kWheelStepLines);
                return true;
            }
            if (button == ftxui::Mouse::WheelDown) {
                scroll_down(kWheelStepLines);
                return true;
            }
        }
        return false;
    }

    ftxui::Element render() {
        using ftxui::flex;
        using ftxui::frame;
        using ftxui::vbox;
        using ftxui::vscroll_indicator;
        std::vector<Tree> trees;
        {
            std::scoped_lock<std::mutex> lock(data_mutex_);
            trees = trees_;
        }
        auto lines = build_tui_lines(trees, format_);
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

    void run_ui() {
        auto screen = ftxui::ScreenInteractive::Fullscreen();
        {
            std::scoped_lock<std::mutex> lock(screen_mutex_);
            exit_loop_ = screen.ExitLoopClosure();
            post_refresh_ = [&screen] {
                screen.PostEvent(ftxui::Event::Custom);
            };
        }
        auto renderer = ftxui::Renderer([this] { return render(); });
        auto component =
            ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
                return on_event(std::move(event));
            });
        screen.Loop(component);
        restore_terminal_state();
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
