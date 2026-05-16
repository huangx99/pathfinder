#include "pathfinder/agent/action_space.h"
#include "pathfinder/foundation/id.h"
#include <set>

namespace pathfinder::agent {

foundation::Result<void> AgentActionCandidate::validateBasic() const {
    if (action_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "action_candidate_id_missing", "action_id must not be empty"));
    }
    if (!foundation::isValidIdString(action_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "action_candidate_id_invalid", "action_id has invalid format: " + action_id.value()));
    }
    if (intent_type == AgentIntentType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "action_candidate_intent_unknown", "intent_type must not be Unknown"));
    }
    // Check that required_target_types does not contain None
    for (const auto& type : required_target_types) {
        if (type == command::ActionTargetType::None) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_invalid_argument, "action_candidate_target_type_none",
                    "required_target_types must not contain None"));
        }
    }
    return foundation::Result<void>::ok();
}

foundation::Result<void> AgentActionSpace::validateBasic() const {
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "action_space_agent_id_missing", "agent_id must not be empty"));
    }
    if (!foundation::isValidIdString(agent_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "action_space_agent_id_invalid", "agent_id has invalid format: " + agent_id.value()));
    }

    // Check for duplicate action_ids
    std::set<std::string> seen;
    for (const auto& candidate : candidates) {
        if (!seen.insert(candidate.action_id.value()).second) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_duplicate, "action_space_duplicate_action",
                    "candidates contains duplicate action_id: " + candidate.action_id.value()));
        }
        auto r = candidate.validateBasic();
        if (r.is_error()) return r;
    }

    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
