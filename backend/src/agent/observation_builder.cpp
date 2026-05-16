#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/cognition/cognition_state.h"
#include "pathfinder/object/world_object.h"
#include "pathfinder/object/types.h"

namespace pathfinder::agent {

using object::ObjectRuntimeFlag;

foundation::Result<ObservationBuildResult> ObservationBuilder::build(
    const ObservationBuildRequest& request) const {

    // Validate request
    auto req_result = request.validateBasic();
    if (req_result.is_error()) {
        return foundation::Result<ObservationBuildResult>::fail(req_result.errors());
    }

    // Validate state has valid basic structure
    auto state_result = request.state->validateBasic();
    if (state_result.is_error()) {
        return foundation::Result<ObservationBuildResult>::fail(state_result.errors());
    }

    // Validate primary_actor_id exists in state
    auto* actor = request.state->actor_store.findActor(request.binding.primary_actor_id);
    if (!actor) {
        return foundation::Result<ObservationBuildResult>::fail(
            foundation::makeError(foundation::ErrorCode::id_missing, "observation_build_actor_not_found",
                "primary_actor_id not found in state: " + request.binding.primary_actor_id.value()));
    }

    ObservationBuildResult result;

    // Build base observation
    result.observation.agent_id = request.binding.agent_id;
    result.observation.observer_actor_id = request.binding.primary_actor_id;
    result.observation.state_version = request.state->metadata.state_version;
    result.observation.observed_tick = request.state->metadata.current_tick;

    // Process visible objects
    for (const auto& obj_id : request.visibility.visible_object_ids) {
        auto* obj = request.state->object_store.findObject(obj_id);
        if (!obj) {
            return foundation::Result<ObservationBuildResult>::fail(
                foundation::makeError(foundation::ErrorCode::id_missing, "observation_build_object_not_found",
                    "visible object not found: " + obj_id.value()));
        }

        // Skip consumed/depleted objects
        if (obj->flag == ObjectRuntimeFlag::Consumed ||
            obj->flag == ObjectRuntimeFlag::Depleted) {
            result.trace.skipped_object_ids.push_back(obj_id);
            result.trace.reason_keys.push_back("object_consumed_or_depleted");
            continue;
        }

        // Build observed object
        AgentObservedObject observed;
        observed.object_id = obj_id;
        observed.known_edible_confidence = 0.0;
        observed.known_usable_confidence = 0.0;
        observed.risk_confidence = 0.0;
        observed.evidence_count = 0;
        observed.can_teach_hint = false;

        // Project cognition claims if requested
        if (request.include_cognition_claims) {
            auto* def = request.state->object_store.findDefinition(obj->definition_id);
            if (def) {
                auto eat_action = foundation::ActionId(std::string("eat"));

                auto* edible_claim = request.state->cognition_state.findEdibleClaim(
                    request.binding.primary_actor_id, obj->definition_id, eat_action);
                if (edible_claim) {
                    observed.known_edible_confidence = edible_claim->confidence;
                    observed.evidence_count = edible_claim->evidence_count;
                    if (edible_claim->confidence >= 0.5) {
                        observed.can_teach_hint = true;
                    }
                }

                auto* harmful_claim = request.state->cognition_state.findHarmfulClaim(
                    request.binding.primary_actor_id, obj->definition_id, eat_action);
                if (harmful_claim) {
                    observed.risk_confidence = harmful_claim->confidence;
                }
            }
        }

        result.observation.objects.push_back(observed);
        result.trace.included_object_ids.push_back(obj_id);
        result.trace.reason_keys.push_back("object_visible_and_interactable");
    }

    // Process threat seeds
    for (const auto& seed : request.visibility.threat_seeds) {
        auto seed_result = seed.validateBasic();
        if (seed_result.is_error()) {
            return foundation::Result<ObservationBuildResult>::fail(seed_result.errors());
        }

        AgentObservedThreat threat;
        threat.source_id = seed.source_id;
        threat.threat_type = seed.threat_type;
        threat.severity = seed.severity;
        threat.confidence = seed.confidence;
        result.observation.threats.push_back(threat);
    }

    // Validate the built observation
    auto obs_result = result.observation.validateBasic();
    if (obs_result.is_error()) {
        return foundation::Result<ObservationBuildResult>::fail(obs_result.errors());
    }

    return foundation::Result<ObservationBuildResult>::ok(std::move(result));
}

} // namespace pathfinder::agent
