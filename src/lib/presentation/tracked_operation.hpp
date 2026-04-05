#ifndef SRC_LIB_PRESENTATION_TRACKED_OPERATION_HPP_
#define SRC_LIB_PRESENTATION_TRACKED_OPERATION_HPP_

#include "core/tracker.hpp"
#include <cstdint>
#include <memory>

enum class DisplayFormat : std::uint8_t;
class OutputView;

class TrackedOperation {
  public:
    TrackedOperation(const std::vector<Tree> &trees, DisplayFormat format);
    ~TrackedOperation();

    Tracker &tracker();
    OutputView &view();

    TrackedOperation(const TrackedOperation &) = delete;
    TrackedOperation &operator=(const TrackedOperation &) = delete;

  private:
    Tracker tracker_;
    std::unique_ptr<OutputView> view_;
};

#endif // SRC_LIB_PRESENTATION_TRACKED_OPERATION_HPP_
