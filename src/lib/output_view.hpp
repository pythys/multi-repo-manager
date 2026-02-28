#ifndef SRC_LIB_OUTPUT_VIEW_HPP_
#define SRC_LIB_OUTPUT_VIEW_HPP_

/**
 * @file output_view.hpp
 * @brief Output rendering abstraction for command execution.
 */

#include "runtime.hpp"
#include "tracker.hpp"
#include <memory>

/** View interface for rendering tracker updates. */
class OutputView {
  public:
    virtual ~OutputView() = default;
    /** Starts background rendering/listening. */
    virtual void start() = 0;
    /** Stops rendering and joins background workers. */
    virtual void stop() = 0;
};

/** Factory for output view implementations (TUI or plain text). */
std::unique_ptr<OutputView>
create_output_view(OutputMode mode, Tracker &tracker);

#endif // SRC_LIB_OUTPUT_VIEW_HPP_
