// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

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

pathfinder::knowledge::KnowledgeClaim defaultKnowledgeFromTemplate(
    const std::string& session_id,
    const pathfinder::knowledge::KnowledgeOwner& owner,
    const DialogDefaultKnowledgeTemplate& config) {
    pathfinder::knowledge::KnowledgeClaim claim;
    claim.knowledge_id = pathfinder::foundation::KnowledgeId("knowledge_default_" + config.template_key + "_" + session_id);
    claim.owner = owner;
    claim.subject.kind = pathfinder::knowledge::KnowledgeSubjectKind::ObjectDefinition;
    claim.subject.subject_id = config.subject_object_key;
    claim.subject.subject_type_key = "world_object";
    if (!config.target_object_key.empty()) claim.subject.related_subject_ids.push_back(config.target_object_key);
    claim.predicate.relation_type = pathfinder::knowledge::KnowledgeRelationType::HasEffect;
    claim.predicate.action_key = config.action_key;
    claim.predicate.effect_key = config.effect_key;
    claim.confidence.confidence = config.confidence;
    claim.confidence.band = pathfinder::knowledge::KnowledgeConfidenceBand::Strong;
    claim.confidence.support_count = 1;
    claim.confidence.last_change_reason_key = "scenario_default_knowledge";
    claim.status = pathfinder::knowledge::KnowledgeStatus::Shared;
    claim.projection_flags.visible_to_player = config.visible_to_player;
    claim.projection_flags.usable_by_ai = config.usable_by_ai;
    claim.projection_flags.usable_for_action = config.usable_for_action;
    claim.reason_keys = config.reason_keys;
    claim.created_tick = Tick(1);
    claim.updated_tick = Tick(1);
    return claim;
}

bool hasDefaultKnowledge(const DialogSessionState& state, const DialogDefaultKnowledgeTemplate& config) {
    return std::any_of(state.recipient_claims.begin(), state.recipient_claims.end(), [&](const pathfinder::knowledge::KnowledgeClaim& claim) {
        return claim.subject.subject_id == config.subject_object_key &&
               claim.predicate.action_key == config.action_key &&
               claim.predicate.effect_key == config.effect_key &&
               (config.target_object_key.empty() || std::find(claim.subject.related_subject_ids.begin(), claim.subject.related_subject_ids.end(), config.target_object_key) != claim.subject.related_subject_ids.end());
    });
}

void ensureScenarioDefaultKnowledge(DialogSessionState& state, const DialogScenario& scenario) {
    for (const auto& config : scenario.default_knowledge_templates) {
        if (config.owner_display_key != state.recipient.display_key) continue;
        if (hasDefaultKnowledge(state, config)) continue;
        state.recipient_claims.push_back(defaultKnowledgeFromTemplate(state.session_id, state.recipient.knowledge_owner, config));
        state.debug_keys.push_back("scenario.default_knowledge." + config.template_key);
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
        runtime.numeric_states["quantity"] = obj.initial_quantity;
        for (const auto& [state_key, value] : obj.initial_numeric_states) {
            runtime.numeric_states[state_key] = value;
        }
        if (obj.object_key == "beast_shadow") {
            runtime.numeric_states["threat_level"] = 0.0;
            runtime.numeric_states["observed_fire"] = 0.0;
            runtime.numeric_states["knows_fire_danger"] = 1.0;
            runtime.numeric_states["flank_waits"] = 0.0;
            runtime.tag_states.push_back("dormant");
            runtime.tag_states.push_back("agent_wildlife");
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
    ensureScenarioDefaultKnowledge(state, scenario);

    return state;
}

} // namespace

Result<DialogSessionState> InMemoryDialogSessionStore::loadOrCreate(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        DialogScenarioCatalog catalog;
        auto scenario_r = catalog.defaultScenario();
        if (scenario_r.is_ok()) ensureScenarioDefaultKnowledge(it->second, scenario_r.value());
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
