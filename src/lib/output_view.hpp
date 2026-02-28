#ifndef SRC_LIB_OUTPUT_VIEW_HPP_
#define SRC_LIB_OUTPUT_VIEW_HPP_

#include "runtime.hpp"
#include "tracker.hpp"
#include <memory>

class OutputView {
  public:
    virtual ~OutputView() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
};

std::unique_ptr<OutputView>
create_output_view(OutputMode mode, Tracker &tracker);

#endif // SRC_LIB_OUTPUT_VIEW_HPP_
