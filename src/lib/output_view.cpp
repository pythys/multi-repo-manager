#include "output_view.hpp"
#include <atomic>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <iostream>
#include <mutex>
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
        if (!repo.messages.empty()) {
            lines.push_back(
                text("  " + repo.messages.back()) |
                color(ftxui::Color::GrayLight));
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

void print_final_report(const Tracker &tracker) {
    const std::vector<Tree> trees = tracker.snapshot();
    std::vector<std::string> lines;
    lines.emplace_back("mrm report");
    for (const auto &tree : trees) {
        lines.emplace_back("");
        lines.emplace_back(tree.root);
        flatten_repo_lines_text(tree.root, tree.repos, lines);
    }
    for (const auto &line : lines) {
        std::cout << line << '\n';
    }
}

class TextView final : public OutputView {
  public:
    explicit TextView(Tracker &tracker) : tracker_(tracker) {}
    ~TextView() override = default;
    void start() override {}
    void stop() override {
        if (stopped_.exchange(true)) {
            return;
        }
        print_final_report(tracker_);
    }

  private:
    Tracker &tracker_;
    std::atomic<bool> stopped_ = false;
};

class TuiView final : public OutputView {
  public:
    explicit TuiView(Tracker &tracker) : tracker_(tracker) {}
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
        print_final_report(tracker_);
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
        using ftxui::bold;
        using ftxui::flex;
        using ftxui::frame;
        using ftxui::hbox;
        using ftxui::separator;
        using ftxui::text;
        using ftxui::vbox;
        using ftxui::vscroll_indicator;
        std::vector<Tree> trees;
        {
            std::scoped_lock<std::mutex> lock(data_mutex_);
            trees = trees_;
        }
        std::vector<ftxui::Element> lines;
        lines.push_back(
            hbox({text("mrm"), separator(), text("live status")}) | bold);
        lines.push_back(text("Scroll to view all items."));
        lines.push_back(ftxui::separator());
        for (const auto &tree : trees) {
            lines.push_back(text(tree.root) | bold);
            flatten_repo_lines(tree.root, tree.repos, lines);
            lines.push_back(text(""));
        }
        line_count_ = lines.size();
        if (line_count_ == 0) {
            selected_line_ = 0;
        } else if (selected_line_ >= line_count_) {
            selected_line_ = line_count_ - 1;
            lines[selected_line_] = lines[selected_line_] | ftxui::focus;
        } else {
            lines[selected_line_] = lines[selected_line_] | ftxui::focus;
        }
        return vbox(
            {vbox(std::move(lines)) | vscroll_indicator | frame | flex});
    }

    void run_ui() {
        auto screen = ftxui::ScreenInteractive::TerminalOutput();
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
create_output_view(OutputMode mode, Tracker &tracker) {
    if (mode == OutputMode::TUI) {
        return std::make_unique<TuiView>(tracker);
    }
    return std::make_unique<TextView>(tracker);
}
