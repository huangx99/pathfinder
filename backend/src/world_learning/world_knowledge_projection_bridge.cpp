#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"

namespace pathfinder::world_learning {

WorldKnowledgeProjectionBridge::ProjectionResult
WorldKnowledgeProjectionBridge::project(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& learned_claims,
    const std::vector<WorldKnowledgeDelta>& knowledge_deltas,
    const std::string& actor_key,
    uint64_t tick) const {
    ProjectionResult result;

    for (const auto& claim : learned_claims) {
        auto patch = claimToPatch(claim, actor_key);
        result.patch.changed_knowledge.push_back(patch);

        auto evt = claimToEvent(claim, actor_key, tick);
        result.events.push_back(evt);
    }

    for (const auto& delta : knowledge_deltas) {
        auto sd = claimToDelta(delta);
        result.state_deltas.push_back(sd);
    }

    result.patch.new_projection_version = tick;
    return result;
}

world_command::KnowledgePatchDto WorldKnowledgeProjectionBridge::claimToPatch(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& actor_key) const {
    world_command::KnowledgePatchDto patch;
    patch.actor_key = actor_key;
    patch.op = world_command::PatchOp::Update;
    patch.fields["knowledge_id"] = claim.knowledge_id.value();
    patch.fields["subject_id"] = claim.subject.subject_id;
    patch.fields["action_key"] = claim.predicate.action_key;
    patch.fields["effect_key"] = claim.predicate.effect_key;
    patch.fields["status"] = pathfinder::knowledge::toString(claim.status);
    patch.fields["teachable"] = claim.teaching_profile.teachable ? "true" : "false";
    patch.fields["usable_by_ai"] = claim.projection_flags.usable_by_ai ? "true" : "false";
    patch.fields["usable_for_action"] = claim.projection_flags.usable_for_action ? "true" : "false";
    patch.fields["confidence"] = std::to_string(claim.confidence.confidence);
    return patch;
}

world_command::WorldEventDto WorldKnowledgeProjectionBridge::claimToEvent(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& actor_key,
    uint64_t tick) const {
    world_command::WorldEventDto evt;
    evt.event_id = claim.knowledge_id.value() + ":event";
    evt.event_kind = "knowledge_formed";
    evt.tick = tick;
    evt.actor_key = actor_key;
    evt.reason_keys = claim.reason_keys;
    return evt;
}

world_command::WorldStateDeltaDto WorldKnowledgeProjectionBridge::claimToDelta(
    const WorldKnowledgeDelta& delta) const {
    world_command::WorldStateDeltaDto sd;
    sd.delta_id = delta.delta_id;
    sd.delta_kind = "knowledge_delta";
    sd.target_ref = delta.owner_key;
    sd.op = world_command::PatchOp::Update;
    sd.fields["knowledge_id"] = delta.knowledge_id.value();
    sd.fields["subject"] = delta.subject_object_key;
    sd.fields["action"] = delta.action_key;
    sd.fields["effect"] = delta.effect_key;
    sd.fields["status"] = delta.status;
    sd.fields["decision"] = delta.decision;
    sd.reason_keys = delta.reason_keys;
    return sd;
}

} // namespace pathfinder::world_learning
