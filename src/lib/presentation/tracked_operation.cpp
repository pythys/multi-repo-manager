#include "presentation/tracked_operation.hpp"
#include "presentation/output_view.hpp"
#include "util/runtime.hpp"

TrackedOperation::TrackedOperation(
    const std::vector<Tree> &trees,
    DisplayFormat format)
    : tracker_(), view_(nullptr) {
    tracker_.populate(trees);
    view_ = create_output_view(detect_output_mode(), format, tracker_);
    view_->start();
}

TrackedOperation::~TrackedOperation() {
    tracker_.close();
    view_->stop();
}

Tracker &TrackedOperation::tracker() {
    return tracker_;
}

OutputView &TrackedOperation::view() {
    return *view_;
}
