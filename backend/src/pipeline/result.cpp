#include "pathfinder/pipeline/result.h"

namespace pathfinder::pipeline {

PipelineResult PipelineResult::successEmpty() {
    PipelineResult result;
    result.status = PipelineStatus::Succeeded;
    result.executed_steps = defaultPipelineSteps();
    return result;
}

pathfinder::foundation::Result<void> PipelineResult::validateBasic() const {
    // Check status is valid
    if (status != PipelineStatus::Succeeded && status != PipelineStatus::Failed) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "PipelineResult status is not Succeeded or Failed"
            )
        );
    }

    // Validate executed_steps order
    for (size_t i = 1; i < executed_steps.size(); ++i) {
        if (executed_steps[i].order <= executed_steps[i-1].order) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "PipelineResult executed_steps order is not strictly increasing"
                )
            );
        }
    }

    // Validate state_changes
    auto sc_result = state_changes.validateBasic();
    if (sc_result.is_error()) {
        return sc_result;
    }

    // Validate events
    auto ev_result = events.validateBasic();
    if (ev_result.is_error()) {
        return ev_result;
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::pipeline
