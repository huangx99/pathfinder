#include "pathfinder/agent/command_adapter.h"

namespace pathfinder::agent {

pathfinder::foundation::CommandId AgentCommandAdapter::makeCommandIdFromDecision(
    const pathfinder::foundation::DecisionId& decision_id) {
    return pathfinder::foundation::CommandId(std::string("command_") + decision_id.value());
}

pathfinder::foundation::ActionId AgentCommandAdapter::makeActionIdFromDecision(
    const pathfinder::foundation::DecisionId& decision_id) {
    return pathfinder::foundation::ActionId(std::string("action_") + decision_id.value());
}

foundation::Result<command::CommandEnvelope> AgentCommandAdapter::toCommandEnvelope(
    const AgentIntent& intent,
    command::CommandSource source,
    pathfinder::foundation::Tick issued_tick) const {

    // Validate source
    if (source == command::CommandSource::Unknown) {
        return foundation::Result<command::CommandEnvelope>::fail(
            foundation::makeError(foundation::ErrorCode::command_invalid_argument, "adapter_source_unknown",
                "source must not be Unknown"));
    }

    // Validate intent first
    auto intent_result = intent.validateBasic();
    if (intent_result.is_error()) {
        return foundation::Result<command::CommandEnvelope>::fail(intent_result.errors());
    }

    // Reject intents from command_supported=false candidates
    if (!intent.command_supported_snapshot) {
        return foundation::Result<command::CommandEnvelope>::fail(
            foundation::makeError(foundation::ErrorCode::common_unsupported, "adapter_command_not_supported",
                "intent comes from a command_supported=false candidate"));
    }

    // Unsupported intent types
    if (intent.intent_type == AgentIntentType::Wait) {
        return foundation::Result<command::CommandEnvelope>::fail(
            foundation::makeError(foundation::ErrorCode::common_unsupported, "adapter_wait_unsupported",
                "Wait intent does not produce a CommandEnvelope"));
    }
    if (intent.intent_type == AgentIntentType::CallGroup) {
        return foundation::Result<command::CommandEnvelope>::fail(
            foundation::makeError(foundation::ErrorCode::common_unsupported, "adapter_call_group_unsupported",
                "CallGroup intent is not supported in P5"));
    }

    // Map AgentIntentType to CommandIntent
    command::CommandIntent command_intent;
    switch (intent.intent_type) {
        case AgentIntentType::Eat:
        case AgentIntentType::Use:
        case AgentIntentType::Explore:
            command_intent = command::CommandIntent::Experiment;
            break;
        case AgentIntentType::AvoidRisk:
        case AgentIntentType::Flee:
            command_intent = command::CommandIntent::AvoidRisk;
            break;
        case AgentIntentType::Teach:
            command_intent = command::CommandIntent::Teach;
            break;
        case AgentIntentType::Combine:
            command_intent = command::CommandIntent::Combine;
            break;
        case AgentIntentType::Fight:
            command_intent = command::CommandIntent::Fight;
            break;
        default:
            return foundation::Result<command::CommandEnvelope>::fail(
                foundation::makeError(foundation::ErrorCode::common_unsupported, "adapter_intent_unsupported",
                    "Unsupported intent type: " + toString(intent.intent_type)));
    }

    // Build CommandEnvelope
    command::CommandEnvelope envelope;
    envelope.command_id = makeCommandIdFromDecision(intent.decision_id);
    envelope.source = source;
    envelope.issued_tick = issued_tick;
    envelope.correlation_id = intent.decision_id.value();

    // Build ActionCommand payload
    // Use semantic action_id from intent if set (e.g. "eat"), otherwise generate from decision_id
    envelope.payload.action_id = intent.action_id.empty()
        ? makeActionIdFromDecision(intent.decision_id)
        : intent.action_id;
    envelope.payload.actor_id = intent.actor_id;
    envelope.payload.targets = intent.targets;
    envelope.payload.intent = command_intent;

    // Validate the constructed envelope
    auto env_result = envelope.validateBasic();
    if (env_result.is_error()) {
        return foundation::Result<command::CommandEnvelope>::fail(env_result.errors());
    }

    return foundation::Result<command::CommandEnvelope>::ok(std::move(envelope));
}

} // namespace pathfinder::agent
