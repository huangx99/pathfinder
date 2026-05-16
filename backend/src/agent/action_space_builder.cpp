#include "pathfinder/agent/action_space_builder.h"

namespace pathfinder::agent {

foundation::Result<ActionSpaceBuildResult> ActionSpaceBuilder::build(
    const ActionSpaceBuildRequest& request) const {

    // Validate request
    auto req_result = request.validateBasic();
    if (req_result.is_error()) {
        return foundation::Result<ActionSpaceBuildResult>::fail(req_result.errors());
    }

    ActionSpaceBuildResult result;

    // Base action space from observation
    result.action_space.agent_id = request.observation.agent_id;
    result.action_space.state_version = request.observation.state_version;

    // Generate Eat candidate for each interactable observed object
    for (const auto& obj : request.observation.objects) {
        // Eat candidate
        AgentActionCandidate eat_candidate;
        eat_candidate.action_id = foundation::ActionId(std::string("eat_") + obj.object_id.value());
        eat_candidate.intent_type = AgentIntentType::Eat;
        eat_candidate.required_target_types.push_back(command::ActionTargetType::Object);
        eat_candidate.command_supported = true;
        eat_candidate.reason_key = "action.try_eat_observed_object";
        result.action_space.candidates.push_back(eat_candidate);

        result.trace.generated_action_ids.push_back(eat_candidate.action_id);
        result.trace.reason_keys.push_back("observed_object_" + obj.object_id.value());

        // Explore candidate (if requested, command_supported=false in P6)
        if (request.include_explore_candidates) {
            AgentActionCandidate explore_candidate;
            explore_candidate.action_id = foundation::ActionId(std::string("explore_") + obj.object_id.value());
            explore_candidate.intent_type = AgentIntentType::Explore;
            explore_candidate.required_target_types.push_back(command::ActionTargetType::Object);
            explore_candidate.command_supported = false;
            explore_candidate.reason_key = "action.explore_observed_object";
            result.action_space.candidates.push_back(explore_candidate);

            result.trace.generated_action_ids.push_back(explore_candidate.action_id);
            result.trace.reason_keys.push_back("explore_" + obj.object_id.value());
        }
    }

    // Generate Flee candidate for fire threats
    // Generate CallGroup candidate for social threats
    for (const auto& threat : request.observation.threats) {
        if (threat.threat_type == AgentThreatType::Fire ||
            threat.threat_type == AgentThreatType::Predator ||
            threat.threat_type == AgentThreatType::Environmental) {
            AgentActionCandidate flee_candidate;
            flee_candidate.action_id = foundation::ActionId(std::string("flee_") + threat.source_id.value());
            flee_candidate.intent_type = AgentIntentType::Flee;
            flee_candidate.required_target_types.push_back(command::ActionTargetType::Entity);
            flee_candidate.command_supported = true;
            flee_candidate.reason_key = "action.flee_from_threat";
            result.action_space.candidates.push_back(flee_candidate);

            result.trace.generated_action_ids.push_back(flee_candidate.action_id);
            result.trace.reason_keys.push_back("flee_threat_" + threat.source_id.value());
        }

        if (threat.threat_type == AgentThreatType::Social) {
            AgentActionCandidate call_group_candidate;
            call_group_candidate.action_id = foundation::ActionId(std::string("call_group_") + threat.source_id.value());
            call_group_candidate.intent_type = AgentIntentType::CallGroup;
            call_group_candidate.command_supported = false;
            call_group_candidate.reason_key = "action.call_group_social_signal";
            result.action_space.candidates.push_back(call_group_candidate);

            result.trace.generated_action_ids.push_back(call_group_candidate.action_id);
            result.trace.reason_keys.push_back("call_group_" + threat.source_id.value());
        }
    }

    // Wait candidate (if requested)
    if (request.include_wait_candidate) {
        AgentActionCandidate wait_candidate;
        wait_candidate.action_id = foundation::ActionId(std::string("wait"));
        wait_candidate.intent_type = AgentIntentType::Wait;
        wait_candidate.command_supported = false;
        wait_candidate.reason_key = "action.wait";
        result.action_space.candidates.push_back(wait_candidate);

        result.trace.generated_action_ids.push_back(wait_candidate.action_id);
        result.trace.reason_keys.push_back("wait");
    }

    // Validate the built action space
    auto as_result = result.action_space.validateBasic();
    if (as_result.is_error()) {
        return foundation::Result<ActionSpaceBuildResult>::fail(as_result.errors());
    }

    return foundation::Result<ActionSpaceBuildResult>::ok(std::move(result));
}

} // namespace pathfinder::agent
