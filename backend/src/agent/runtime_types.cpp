#include "pathfinder/agent/runtime_types.h"

namespace pathfinder::agent {

std::string toString(AgentRuntimeStatus status) {
    switch (status) {
        case AgentRuntimeStatus::Unknown: return "Unknown";
        case AgentRuntimeStatus::ObservationBuilt: return "ObservationBuilt";
        case AgentRuntimeStatus::ActionSpaceBuilt: return "ActionSpaceBuilt";
        case AgentRuntimeStatus::DecisionMade: return "DecisionMade";
        case AgentRuntimeStatus::CommandCreated: return "CommandCreated";
        case AgentRuntimeStatus::SubmitSkipped: return "SubmitSkipped";
        case AgentRuntimeStatus::PipelineSucceeded: return "PipelineSucceeded";
        case AgentRuntimeStatus::PipelineFailed: return "PipelineFailed";
        case AgentRuntimeStatus::NoDecision: return "NoDecision";
        case AgentRuntimeStatus::Failed: return "Failed";
    }
    return "Unknown";
}

foundation::Result<AgentRuntimeStatus> agentRuntimeStatusFromString(const std::string& str) {
    if (str == "Unknown") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::Unknown);
    if (str == "ObservationBuilt") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::ObservationBuilt);
    if (str == "ActionSpaceBuilt") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::ActionSpaceBuilt);
    if (str == "DecisionMade") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::DecisionMade);
    if (str == "CommandCreated") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::CommandCreated);
    if (str == "SubmitSkipped") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::SubmitSkipped);
    if (str == "PipelineSucceeded") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::PipelineSucceeded);
    if (str == "PipelineFailed") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::PipelineFailed);
    if (str == "NoDecision") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::NoDecision);
    if (str == "Failed") return foundation::Result<AgentRuntimeStatus>::ok(AgentRuntimeStatus::Failed);
    return foundation::Result<AgentRuntimeStatus>::fail(
        foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "runtime_status_unknown",
            "unknown AgentRuntimeStatus: " + str));
}

foundation::Result<void> AgentTickRequest::validateBasic() const {
    if (!state) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "tick_request_null_state",
                "state must not be null"));
    }
    auto bind_result = binding.validateBasic();
    if (bind_result.is_error()) return bind_result;
    auto vis_result = visibility.validateBasic();
    if (vis_result.is_error()) return vis_result;
    if (command_source == command::CommandSource::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "tick_request_source_unknown",
                "command_source must not be Unknown"));
    }
    if (options.submit_to_pipeline && options.submit_action_allowlist.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "tick_request_empty_allowlist",
                "submit_to_pipeline=true requires non-empty submit_action_allowlist"));
    }
    return foundation::Result<void>::ok();
}

foundation::Result<void> AgentTickResult::validateBasic() const {
    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
