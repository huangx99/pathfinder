#include "pathfinder/agent/intent.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::agent {

foundation::Result<void> AgentIntent::validateBasic() const {
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "intent_agent_id_missing", "agent_id must not be empty"));
    }
    if (!foundation::isValidIdString(agent_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "intent_agent_id_invalid", "agent_id has invalid format: " + agent_id.value()));
    }
    if (decision_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "intent_decision_id_missing", "decision_id must not be empty"));
    }
    if (!foundation::isValidIdString(decision_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "intent_decision_id_invalid", "decision_id has invalid format: " + decision_id.value()));
    }
    if (actor_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "intent_actor_id_missing", "actor_id must not be empty"));
    }
    if (!foundation::isValidIdString(actor_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "intent_actor_id_invalid", "actor_id has invalid format: " + actor_id.value()));
    }
    if (intent_type == AgentIntentType::Unknown) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_enum_unknown, "intent_type_unknown", "intent_type must not be Unknown"));
    }
    if (confidence < 0.0 || confidence > 1.0) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::validation_value_out_of_range, "intent_confidence_out_of_range",
                "confidence must be in [0.0, 1.0], got " + std::to_string(confidence)));
    }
    if (reason_key.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::command_missing_required_field, "intent_reason_key_empty", "reason_key must not be empty"));
    }

    // For Eat, Use, Teach, Combine, Fight: targets must be non-empty
    if (intent_type == AgentIntentType::Eat ||
        intent_type == AgentIntentType::Use ||
        intent_type == AgentIntentType::Teach ||
        intent_type == AgentIntentType::Combine ||
        intent_type == AgentIntentType::Fight) {
        if (targets.empty()) {
            return foundation::Result<void>::fail(
                foundation::makeError(foundation::ErrorCode::command_missing_required_field, "intent_targets_empty",
                    "targets must not be empty for intent type " + toString(intent_type)));
        }
    }

    // Validate action_id format if non-empty
    if (!action_id.empty() && !foundation::isValidIdString(action_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "intent_action_id_invalid", "action_id has invalid format: " + action_id.value()));
    }

    // Validate each target
    for (const auto& target : targets) {
        auto r = target.validateBasic();
        if (r.is_error()) return r;
    }

    return foundation::Result<void>::ok();
}

foundation::Result<void> AgentDecision::validateBasic() const {
    if (decision_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "decision_id_missing", "decision_id must not be empty"));
    }
    if (!foundation::isValidIdString(decision_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "decision_id_invalid", "decision_id has invalid format: " + decision_id.value()));
    }
    if (agent_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "decision_agent_id_missing", "agent_id must not be empty"));
    }
    if (!foundation::isValidIdString(agent_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "decision_agent_id_invalid", "agent_id has invalid format: " + agent_id.value()));
    }
    if (action_id.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "decision_action_id_missing", "action_id must not be empty"));
    }
    if (!foundation::isValidIdString(action_id.value())) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::id_invalid_format, "decision_action_id_invalid", "action_id has invalid format: " + action_id.value()));
    }
    if (reason_key.empty()) {
        return foundation::Result<void>::fail(
            foundation::makeError(foundation::ErrorCode::command_missing_required_field, "decision_reason_key_empty", "reason_key must not be empty"));
    }
    auto r = selected_intent.validateBasic();
    if (r.is_error()) return r;

    return foundation::Result<void>::ok();
}

} // namespace pathfinder::agent
