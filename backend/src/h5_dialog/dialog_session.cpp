#include "pathfinder/h5_dialog/dialog_session.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include <algorithm>

namespace pathfinder::h5_dialog {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::EntityId;
using pathfinder::knowledge::KnowledgeOwner;
using pathfinder::knowledge::KnowledgeOwnerKind;
using pathfinder::memory::MemoryOwner;
using pathfinder::memory::MemoryOwnerKind;
using pathfinder::cognition::CognitionSubject;
using pathfinder::cognition::CognitionSubjectKind;

namespace {

pathfinder::knowledge::KnowledgeClaim defaultCompanionTorchKnowledge(
    const std::string& session_id,
    const pathfinder::knowledge::KnowledgeOwner& owner) {
    pathfinder::knowledge::KnowledgeClaim claim;
    claim.knowledge_id = pathfinder::foundation::KnowledgeId("knowledge_companion_default_torch_repel_" + session_id);
    claim.owner = owner;
    claim.subject.kind = pathfinder::knowledge::KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = "torch";
    claim.subject.subject_type_key = "world_object";
    claim.subject.related_subject_ids.push_back("beast_shadow");
    claim.predicate.relation_type = pathfinder::knowledge::KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = "use";
    claim.predicate.effect_key = "repel_beast";
    claim.confidence.confidence = 0.85;
    claim.confidence.band = pathfinder::knowledge::KnowledgeConfidenceBand::Strong;
    claim.confidence.support_count = 1;
    claim.confidence.last_change_reason_key = "default_companion_survival_hint";
    claim.status = pathfinder::knowledge::KnowledgeStatus::Shared;
    claim.projection_flags.visible_to_player = true;
    claim.projection_flags.usable_by_ai = true;
    claim.projection_flags.usable_for_action = true;
    claim.reason_keys = {"default_companion_torch_repel", "playtest_helper"};
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

bool hasDefaultCompanionTorchKnowledge(const DialogSessionState& state) {
    return std::any_of(
        state.recipient_claims.begin(),
        state.recipient_claims.end(),
        [](const pathfinder::knowledge::KnowledgeClaim& claim) {
            return claim.subject.subject_id == "torch" &&
                   claim.predicate.action_key == "use" &&
                   claim.predicate.effect_key == "repel_beast" &&
                   std::find(
                       claim.subject.related_subject_ids.begin(),
                       claim.subject.related_subject_ids.end(),
                       "beast_shadow") != claim.subject.related_subject_ids.end();
        });
}

void ensureSessionDefaults(DialogSessionState& state) {
    if (!hasDefaultCompanionTorchKnowledge(state)) {
        state.recipient_claims.push_back(defaultCompanionTorchKnowledge(state.session_id, state.recipient.knowledge_owner));
        state.debug_keys.push_back("migration.default_companion_torch_knowledge");
    }
}

DialogSessionState createNewSession(const std::string& session_id) {
    DialogScenarioCatalog catalog;
    auto scenario_r = catalog.defaultScenario();
    DialogScenario scenario = scenario_r.value();

    DialogSessionState state;
    state.session_id = session_id;
    state.scenario_key = scenario.scenario_key;
    state.turn_index = 0;
    state.current_tick = Tick(1);

    for (const auto& obj : scenario.objects) {
        if (obj.visibility == DialogObjectVisibility::Visible || obj.visibility == DialogObjectVisibility::Mentioned) {
            state.visible_object_keys.push_back(obj.object_key);
        }
        DialogObjectRuntimeState runtime;
        runtime.object_key = obj.object_key;
        runtime.tag_states = obj.safe_tags;
        runtime.numeric_states["quantity"] = 1.0;
        if (obj.object_key == "red_berry") runtime.numeric_states["quantity"] = 3.0;
        if (obj.object_key == "decayed_red_berry") runtime.numeric_states["quantity"] = 6.0;
        if (obj.object_key == "bitter_leaf") runtime.numeric_states["quantity"] = 2.0;
        if (obj.object_key == "wood") runtime.numeric_states["quantity"] = 2.0;
        if (obj.object_key == "dry_grass") runtime.numeric_states["quantity"] = 2.0;
        if (obj.object_key == "torch" || obj.object_key == "camp_fire") runtime.numeric_states["quantity"] = 0.0;
        if (obj.object_key == "beast_shadow") {
            runtime.numeric_states["threat_level"] = 0.0;
            runtime.numeric_states["observed_fire"] = 0.0;
            runtime.numeric_states["knows_fire_danger"] = 1.0;
            runtime.tag_states.push_back("dormant");
            runtime.tag_states.push_back("agent_wildlife");
        }
        if (obj.object_key == "axe") {
            runtime.numeric_states["sharpness"] = 3.0;
        }
        state.object_runtime_states[obj.object_key] = runtime;
    }

    // Actor: pioneer
    EntityId actor_id("pioneer_" + session_id);
    state.actor.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, actor_id, {}, {}, "pioneer"};
    state.actor.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, actor_id, {}, "pioneer"};
    state.actor.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, actor_id, std::nullopt};
    state.actor.display_key = "pioneer";

    // Recipient: companion
    EntityId recip_id("companion_" + session_id);
    state.recipient.knowledge_owner = KnowledgeOwner{KnowledgeOwnerKind::Actor, recip_id, {}, {}, "companion"};
    state.recipient.memory_owner = MemoryOwner{MemoryOwnerKind::Actor, recip_id, {}, "companion"};
    state.recipient.cognition_subject = CognitionSubject{CognitionSubjectKind::Actor, recip_id, std::nullopt};
    state.recipient.display_key = "companion";
    ensureSessionDefaults(state);

    return state;
}

} // namespace

Result<DialogSessionState> InMemoryDialogSessionStore::loadOrCreate(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        ensureSessionDefaults(it->second);
        return Result<DialogSessionState>::ok(it->second);
    }
    auto state = createNewSession(session_id);
    sessions_[session_id] = state;
    return Result<DialogSessionState>::ok(state);
}

Result<void> InMemoryDialogSessionStore::save(const DialogSessionState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[state.session_id] = state;
    return Result<void>::ok();
}

Result<void> InMemoryDialogSessionStore::reset(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
    auto state = createNewSession(session_id);
    sessions_[session_id] = state;
    return Result<void>::ok();
}

} // namespace pathfinder::h5_dialog
