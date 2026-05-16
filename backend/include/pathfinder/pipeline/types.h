#pragma once

#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::pipeline {

// Pipeline stages
enum class PipelineStage {
    AcceptCommand,
    ValidateCommand,
    PrepareContext,
    ResolveRules,
    ValidateStateChanges,
    ApplyStateChanges,
    BuildEvents,
    EmitEvents,
    Finish
};

std::string toString(PipelineStage stage);
pathfinder::foundation::Result<PipelineStage> pipelineStageFromString(const std::string& str);

// Pipeline status
enum class PipelineStatus {
    NotStarted,
    Running,
    Succeeded,
    Failed
};

std::string toString(PipelineStatus status);
pathfinder::foundation::Result<PipelineStatus> pipelineStatusFromString(const std::string& str);

// Pipeline step
struct PipelineStep {
    PipelineStage stage;
    int order;
    std::string name;
};

// Get default pipeline steps
std::vector<PipelineStep> defaultPipelineSteps();

} // namespace pathfinder::pipeline
