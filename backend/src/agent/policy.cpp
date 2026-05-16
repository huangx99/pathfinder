#include "pathfinder/agent/policy.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::agent {

foundation::Result<void> AgentPolicyRequest::validateBasic() const {
    auto bind_result = binding.validateBasic();
    if (bind_result.is_error()) return bind_result;

    auto obs_result = observation.validateBasic();
    if (obs_result.is_error()) return obs_result;

    auto as_result = action_space.validateBasic();
    if (as_result.is_error()) return as_result;

    return foundation::Result<void>::ok();
}

foundation::Result<AgentDecision> FirstSupportedPolicy::decide(
    const AgentPolicyRequest& request) const {

    auto req_result = request.validateBasic();
    if (req_result.is_error()) {
        return foundation::Result<AgentDecision>::fail(req_result.errors());
    }

    // Find first command_supported=true candidate
    // If submit_action_allowlist is non-empty, prioritize candidates whose
    // command_action_id is in the allowlist
    int selected_index = -1;
    const AgentActionCandidate* selected = nullptr;

    if (!request.submit_action_allowlist.empty()) {
        // First pass: find first candidate in allowlist
        for (size_t i = 0; i < request.action_space.candidates.size(); ++i) {
            const auto& c = request.action_space.candidates[i];
            if (!c.command_supported) continue;
            for (const auto& allowed : request.submit_action_allowlist) {
                if (c.command_action_id == allowed || c.action_id == allowed) {
                    selected_index = static_cast<int>(i);
                    selected = &c;
                    break;
                }
            }
            if (selected) break;
        }
    }

    // Second pass (fallback): find first command_supported=true candidate
    if (!selected) {
        for (size_t i = 0; i < request.action_space.candidates.size(); ++i) {
            const auto& c = request.action_space.candidates[i];
            if (c.command_supported) {
                selected_index = static_cast<int>(i);
                selected = &c;
                break;
            }
        }
    }

    if (!selected) {
        return foundation::Result<AgentDecision>::fail(
            foundation::makeError(foundation::ErrorCode::validation_failed, "policy_no_supported_candidate",
                "no command_supported=true candidate available"));
    }

    // Build deterministic decision_id
    std::string decision_id_str = "decision_" + request.binding.agent_id.value()
        + "_" + std::to_string(request.decision_tick.value())
        + "_" + std::to_string(selected_index);

    // Build AgentIntent
    AgentIntent intent;
    intent.agent_id = request.binding.agent_id;
    intent.decision_id = foundation::DecisionId(decision_id_str);
    intent.actor_id = request.binding.primary_actor_id;
    intent.intent_type = selected->intent_type;
    intent.action_id = selected->command_action_id.empty()
        ? selected->action_id
        : selected->command_action_id;
    intent.targets = selected->suggested_targets;
    intent.command_supported_snapshot = selected->command_supported;
    intent.reason_key = selected->reason_key;
    intent.confidence = 0.8;

    // Build AgentDecision
    AgentDecision decision;
    decision.decision_id = intent.decision_id;
    decision.agent_id = intent.agent_id;
    decision.selected_intent = std::move(intent);
    decision.observation_state_version = request.observation.state_version;
    decision.action_id = selected->command_action_id.empty()
        ? selected->action_id
        : selected->command_action_id;
    decision.reason_key = selected->reason_key;

    auto dec_result = decision.validateBasic();
    if (dec_result.is_error()) {
        return foundation::Result<AgentDecision>::fail(dec_result.errors());
    }

    return foundation::Result<AgentDecision>::ok(std::move(decision));
}

} // namespace pathfinder::agent
