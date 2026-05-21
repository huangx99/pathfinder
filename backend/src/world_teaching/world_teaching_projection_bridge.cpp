#include "pathfinder/world_teaching/world_teaching_projection_bridge.h"

namespace pathfinder::world_teaching {

WorldTeachingProjectionBridge::ProjectionResult WorldTeachingProjectionBridge::project(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& recipient_claims,
    const pathfinder::knowledge::KnowledgeClaim* source_claim,
    const std::string& teacher_actor_key,
    const std::string& recipient_actor_key,
    const std::string& source_knowledge_id,
    WorldTeachingDecision decision,
    uint64_t tick) const {

    ProjectionResult result;
    result.patch.new_projection_version = tick;

    // Event: knowledge_taught
    result.events.push_back(makeTeachingEvent(teacher_actor_key, recipient_actor_key, source_knowledge_id, tick));

    // Projection patches and state deltas for recipient claims
    for (const auto& claim : recipient_claims) {
        // Only patch claims that were created/updated in this teaching session
        bool is_new_or_updated = true;
        if (source_claim != nullptr && claim.knowledge_id == source_claim->knowledge_id) {
            // If recipient already had this exact knowledge_id, it would be an update.
            // In normal propagation, recipient gets a NEW knowledge_id.
        }
        (void)is_new_or_updated; // all propagated claims are relevant

        result.patch.changed_knowledge.push_back(claimToPatch(claim, recipient_actor_key));
        result.state_deltas.push_back(makeTeachingDelta(claim, recipient_actor_key, source_knowledge_id, decision));
    }

    return result;
}

world_command::KnowledgePatchDto WorldTeachingProjectionBridge::claimToPatch(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& recipient_actor_key) const {
    world_command::KnowledgePatchDto patch;
    patch.actor_key = recipient_actor_key;
    patch.op = world_command::PatchOp::Update;
    patch.fields["knowledge_id"] = claim.knowledge_id.value();
    patch.fields["subject_id"] = claim.subject.subject_id;
    patch.fields["action_key"] = claim.predicate.action_key;
    patch.fields["effect_key"] = claim.predicate.effect_key;
    patch.fields["status"] = pathfinder::knowledge::toString(claim.status);
    patch.fields["confidence"] = std::to_string(claim.confidence.confidence);
    patch.fields["teachable"] = claim.teaching_profile.teachable ? "true" : "false";
    patch.fields["usable_by_ai"] = claim.projection_flags.usable_by_ai ? "true" : "false";
    patch.fields["usable_for_action"] = claim.projection_flags.usable_for_action ? "true" : "false";
    return patch;
}

world_command::WorldEventDto WorldTeachingProjectionBridge::makeTeachingEvent(
    const std::string& teacher_actor_key,
    const std::string& recipient_actor_key,
    const std::string& source_knowledge_id,
    uint64_t tick) const {
    world_command::WorldEventDto evt;
    evt.event_id = "evt_teaching_" + source_knowledge_id + "_" + std::to_string(tick);
    evt.event_kind = "knowledge_taught";
    evt.tick = tick;
    evt.actor_key = teacher_actor_key;
    evt.target_entity_id = recipient_actor_key;
    evt.reason_keys = {"p50_teaching", source_knowledge_id};
    return evt;
}

world_command::WorldStateDeltaDto WorldTeachingProjectionBridge::makeTeachingDelta(
    const pathfinder::knowledge::KnowledgeClaim& claim,
    const std::string& recipient_actor_key,
    const std::string& source_knowledge_id,
    WorldTeachingDecision decision) const {
    world_command::WorldStateDeltaDto sd;
    sd.delta_id = "delta_teaching_" + claim.knowledge_id.value();
    sd.delta_kind = "recipient_knowledge_delta";
    sd.target_ref = recipient_actor_key;
    sd.op = world_command::PatchOp::Update;
    sd.fields["knowledge_id"] = claim.knowledge_id.value();
    sd.fields["source_knowledge_id"] = source_knowledge_id;
    sd.fields["decision"] = toString(decision);
    sd.reason_keys = {"p50_teaching", source_knowledge_id};
    return sd;
}

} // namespace pathfinder::world_teaching
