#ifndef SRC_LIB_TERMINAL_SCREEN_HPP_
#define SRC_LIB_TERMINAL_SCREEN_HPP_

#include "tracker.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

class TerminalScreen : public TreeObserver {
  public:
    TerminalScreen() = default;
    void update() override {
        render();
    }
    static void render() {
        using ftxui::bold;
        using ftxui::center;
        using ftxui::Component;
        using ftxui::Renderer;
        using ftxui::ScreenInteractive;
        using ftxui::text;

        auto renderer = Renderer(
            [] { return text("Hello, TerminalScreen!") | center | bold; });

        auto screen = ScreenInteractive::FitComponent();
        // screen.Loop(renderer);
    }
};

#endif // SRC_LIB_TERMINAL_SCREEN_HPP_
