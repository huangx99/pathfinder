#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/pipeline/types.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/event/event_stream.h"
#include <vector>

namespace pathfinder::pipeline {

// Pipeline result
struct PipelineResult {
    PipelineStatus status;
    std::vector<PipelineStep> executed_steps;
    pathfinder::state::StateChangeSet state_changes;
    pathfinder::event::EventStream events;
    std::vector<pathfinder::foundation::ErrorDetail> errors;

    // Create success empty result
    static PipelineResult successEmpty();

    // Check if has errors
    bool hasErrors() const { return !errors.empty(); }

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::pipeline
