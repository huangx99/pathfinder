#include "pathfinder/pipeline/types.h"

namespace pathfinder::pipeline {

// PipelineStage
std::string toString(PipelineStage stage) {
    switch (stage) {
        case PipelineStage::AcceptCommand: return "accept_command";
        case PipelineStage::ValidateCommand: return "validate_command";
        case PipelineStage::PrepareContext: return "prepare_context";
        case PipelineStage::ResolveRules: return "resolve_rules";
        case PipelineStage::ValidateStateChanges: return "validate_state_changes";
        case PipelineStage::ApplyStateChanges: return "apply_state_changes";
        case PipelineStage::BuildEvents: return "build_events";
        case PipelineStage::EmitEvents: return "emit_events";
        case PipelineStage::Finish: return "finish";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<PipelineStage> pipelineStageFromString(const std::string& str) {
    if (str == "accept_command") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::AcceptCommand);
    if (str == "validate_command") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::ValidateCommand);
    if (str == "prepare_context") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::PrepareContext);
    if (str == "resolve_rules") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::ResolveRules);
    if (str == "validate_state_changes") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::ValidateStateChanges);
    if (str == "apply_state_changes") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::ApplyStateChanges);
    if (str == "build_events") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::BuildEvents);
    if (str == "emit_events") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::EmitEvents);
    if (str == "finish") return pathfinder::foundation::Result<PipelineStage>::ok(PipelineStage::Finish);
    return pathfinder::foundation::Result<PipelineStage>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown PipelineStage: " + str
        )
    );
}

// PipelineStatus
std::string toString(PipelineStatus status) {
    switch (status) {
        case PipelineStatus::NotStarted: return "not_started";
        case PipelineStatus::Running: return "running";
        case PipelineStatus::Succeeded: return "succeeded";
        case PipelineStatus::Failed: return "failed";
        default: return "unknown";
    }
}

pathfinder::foundation::Result<PipelineStatus> pipelineStatusFromString(const std::string& str) {
    if (str == "not_started") return pathfinder::foundation::Result<PipelineStatus>::ok(PipelineStatus::NotStarted);
    if (str == "running") return pathfinder::foundation::Result<PipelineStatus>::ok(PipelineStatus::Running);
    if (str == "succeeded") return pathfinder::foundation::Result<PipelineStatus>::ok(PipelineStatus::Succeeded);
    if (str == "failed") return pathfinder::foundation::Result<PipelineStatus>::ok(PipelineStatus::Failed);
    return pathfinder::foundation::Result<PipelineStatus>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "Unknown PipelineStatus: " + str
        )
    );
}

// Default pipeline steps
std::vector<PipelineStep> defaultPipelineSteps() {
    return {
        {PipelineStage::AcceptCommand, 10, "Accept Command"},
        {PipelineStage::ValidateCommand, 20, "Validate Command"},
        {PipelineStage::PrepareContext, 30, "Prepare Context"},
        {PipelineStage::ResolveRules, 40, "Resolve Rules"},
        {PipelineStage::ValidateStateChanges, 50, "Validate State Changes"},
        {PipelineStage::ApplyStateChanges, 60, "Apply State Changes"},
        {PipelineStage::BuildEvents, 70, "Build Events"},
        {PipelineStage::EmitEvents, 80, "Emit Events"},
        {PipelineStage::Finish, 90, "Finish"}
    };
}

} // namespace pathfinder::pipeline
