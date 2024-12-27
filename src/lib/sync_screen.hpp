#ifndef SRC_LIB_SYNC_SCREEN_HPP_
#define SRC_LIB_SYNC_SCREEN_HPP_

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include "tracker.hpp"

class SyncScreen : public TreeObserver {
 public:
    SyncScreen() = default;
        void update() override {
            render();
        }
        static void render() {
            using ftxui::Renderer;
            using ftxui::Component;
            using ftxui::ScreenInteractive;
            using ftxui::text;
            using ftxui::center;
            using ftxui::bold;

            auto renderer = Renderer([] {
                return text("Hello, SyncScreen!") | center | bold;
            });

            auto screen = ScreenInteractive::FitComponent();
            // screen.Loop(renderer);
        }
};

#endif  // SRC_LIB_SYNC_SCREEN_HPP_
