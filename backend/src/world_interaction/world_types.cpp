#include "pathfinder/world_interaction/world_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_interaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

namespace {

Result<void> fail(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

bool containsForbiddenText(const std::string& text) {
    return text.find("hidden_truth") != std::string::npos ||
           text.find("raw_state") != std::string::npos ||
           text.find("debug_only") != std::string::npos ||
           text.find("internal_error") != std::string::npos;
}

bool containsForbiddenText(const std::vector<std::string>& values) {
    for (const auto& value : values) {
        if (containsForbiddenText(value)) return true;
    }
    return false;
}

bool hasKnownThreatTarget(WorldChangeKind kind) {
    return kind == WorldChangeKind::ThreatLevelChanged || kind == WorldChangeKind::ThreatResolved;
}

bool hasKnownObjectTarget(WorldChangeKind kind) {
    return kind == WorldChangeKind::ObjectQuantityChanged ||
           kind == WorldChangeKind::ObjectConsumed ||
           kind == WorldChangeKind::ObjectGenerated ||
           kind == WorldChangeKind::ObjectStateChanged;
}

} // namespace

std::string toString(WorldObjectInstanceKind kind) {
    switch (kind) {
        case WorldObjectInstanceKind::ResourceStack: return "resource_stack";
        case WorldObjectInstanceKind::ToolInstance: return "tool_instance";
        case WorldObjectInstanceKind::ConsumableInstance: return "consumable_instance";
        case WorldObjectInstanceKind::GeneratedInstance: return "generated_instance";
        case WorldObjectInstanceKind::ThreatMarker: return "threat_marker";
        case WorldObjectInstanceKind::CompanionHeldItem: return "companion_held_item";
        case WorldObjectInstanceKind::TestOnly: return "test_only";
        case WorldObjectInstanceKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(WorldChangeKind kind) {
    switch (kind) {
        case WorldChangeKind::ObjectQuantityChanged: return "object_quantity_changed";
        case WorldChangeKind::ObjectConsumed: return "object_consumed";
        case WorldChangeKind::ObjectGenerated: return "object_generated";
        case WorldChangeKind::ObjectStateChanged: return "object_state_changed";
        case WorldChangeKind::ActorNeedChanged: return "actor_need_changed";
        case WorldChangeKind::ActorConditionChanged: return "actor_condition_changed";
        case WorldChangeKind::ThreatLevelChanged: return "threat_level_changed";
        case WorldChangeKind::ThreatResolved: return "threat_resolved";
        case WorldChangeKind::ObjectiveProgressed: return "objective_progressed";
        case WorldChangeKind::AgentActionQueued: return "agent_action_queued";
        case WorldChangeKind::AgentActionExecuted: return "agent_action_executed";
        case WorldChangeKind::StoryOutcomeReached: return "story_outcome_reached";
        case WorldChangeKind::TestOnly: return "test_only";
        case WorldChangeKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(InteractionFailureKind kind) {
    switch (kind) {
        case InteractionFailureKind::MissingObject: return "missing_object";
        case InteractionFailureKind::InsufficientQuantity: return "insufficient_quantity";
        case InteractionFailureKind::MissingTarget: return "missing_target";
        case InteractionFailureKind::TargetMismatch: return "target_mismatch";
        case InteractionFailureKind::ConditionNotMet: return "condition_not_met";
        case InteractionFailureKind::ToolUnavailable: return "tool_unavailable";
        case InteractionFailureKind::KnowledgeNotKnown: return "knowledge_not_known";
        case InteractionFailureKind::ThreatNotActive: return "threat_not_active";
        case InteractionFailureKind::CompanionCannotAct: return "companion_cannot_act";
        case InteractionFailureKind::ForbiddenByAudience: return "forbidden_by_audience";
        case InteractionFailureKind::TestOnly: return "test_only";
        case InteractionFailureKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(AgentAutonomyActionKind kind) {
    switch (kind) {
        case AgentAutonomyActionKind::None: return "none";
        case AgentAutonomyActionKind::HoldTorch: return "hold_torch";
        case AgentAutonomyActionKind::GatherMaterial: return "gather_material";
        case AgentAutonomyActionKind::WarnDanger: return "warn_danger";
        case AgentAutonomyActionKind::MaintainTool: return "maintain_tool";
        case AgentAutonomyActionKind::AvoidHazard: return "avoid_hazard";
        case AgentAutonomyActionKind::ApproachTarget: return "approach_target";
        case AgentAutonomyActionKind::AttackTarget: return "attack_target";
        case AgentAutonomyActionKind::FollowActor: return "follow_actor";
        case AgentAutonomyActionKind::TeachBack: return "teach_back";
        case AgentAutonomyActionKind::TestOnly: return "test_only";
        case AgentAutonomyActionKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(ThreatEventPhase phase) {
    switch (phase) {
        case ThreatEventPhase::Dormant: return "dormant";
        case ThreatEventPhase::Foreshadowing: return "foreshadowing";
        case ThreatEventPhase::Approaching: return "approaching";
        case ThreatEventPhase::Confronting: return "confronting";
        case ThreatEventPhase::Mitigated: return "mitigated";
        case ThreatEventPhase::Resolved: return "resolved";
        case ThreatEventPhase::Failed: return "failed";
        case ThreatEventPhase::TestOnly: return "test_only";
        case ThreatEventPhase::Unknown: return "unknown";
    }
    return "unknown";
}

Result<void> WorldObjectInstance::validateBasic() const {
    if (instance_id.empty()) return fail("world object instance id empty");
    if (definition_key.empty()) return fail("world object definition key empty");
    if (display_name_zh_cn.empty()) return fail("world object display name empty");
    if (kind == WorldObjectInstanceKind::Unknown) return fail("world object kind unknown");
    if (quantity < 0) return fail("world object quantity negative");
    for (const auto& [actor_key, quantity_value] : actor_quantities) {
        if (actor_key.empty() || quantity_value < 0) return fail("world object actor quantity invalid");
        if (containsForbiddenText(actor_key)) return fail("world object actor quantity contains forbidden text");
    }
    if (containsForbiddenText(display_name_zh_cn) || containsForbiddenText(owner_actor_key) ||
        containsForbiddenText(permitted_actor_keys) || containsForbiddenText(state_tags)) return fail("world object contains forbidden text");
    return Result<void>::ok();
}

Result<void> WorldActorRuntimeState::validateBasic() const {
    if (actor_key.empty()) return fail("world actor key empty");
    if (display_name_zh_cn.empty()) return fail("world actor display empty");
    if (trust < 0.0 || trust > 1.0) return fail("world actor trust out of range");
    if (containsForbiddenText(display_name_zh_cn) || containsForbiddenText(known_effect_keys)) return fail("world actor contains forbidden text");
    for (const auto& claim : known_claims) {
        auto valid = claim.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> WorldThreatRuntimeState::validateBasic() const {
    if (threat_key.empty()) return fail("world threat key empty");
    if (display_name_zh_cn.empty()) return fail("world threat display empty");
    if (phase == ThreatEventPhase::Unknown) return fail("world threat phase unknown");
    if (level < 0.0 || level > 100.0) return fail("world threat level out of range");
    if (containsForbiddenText(display_name_zh_cn) || containsForbiddenText(observed_object_keys)) return fail("world threat contains forbidden text");
    return Result<void>::ok();
}

Result<void> WorldSnapshot::validateBasic() const {
    if (snapshot_id.empty()) return fail("world snapshot id empty");
    if (scenario_key.empty()) return fail("world snapshot scenario empty");
    for (const auto& [key, object] : objects_by_key) {
        if (key != object.definition_key) return fail("world object map key mismatch");
        auto result = object.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& [key, actor] : actors_by_key) {
        if (key != actor.actor_key) return fail("world actor map key mismatch");
        auto result = actor.validateBasic();
        if (result.is_error()) return result;
    }
    for (const auto& [key, threat] : threats_by_key) {
        if (key != threat.threat_key) return fail("world threat map key mismatch");
        auto result = threat.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> WorldInteractionInput::validateBasic() const {
    if (interaction_id.empty()) return fail("world interaction id empty");
    if (actor_key.empty()) return fail("world interaction actor empty");
    if (object_key.empty()) return fail("world interaction object empty");
    if (action == pathfinder::h5_dialog::DialogActionKind::Unknown) return fail("world interaction action unknown");
    if (effect_key.empty()) return fail("world interaction effect empty");
    return Result<void>::ok();
}

Result<void> WorldChange::validateBasic() const {
    if (change_id.empty()) return fail("world change id empty");
    if (kind == WorldChangeKind::Unknown) return fail("world change kind unknown");
    if (player_summary_zh_cn.empty()) return fail("world change summary empty");
    if (containsForbiddenText(player_summary_zh_cn) || containsForbiddenText(reason_keys)) return fail("world change contains forbidden text");
    if (hasKnownObjectTarget(kind) && !target_instance_id.has_value()) return fail("world object change missing target");
    if (hasKnownThreatTarget(kind) && !target_threat_key.has_value()) return fail("world threat change missing target");
    if (kind == WorldChangeKind::ObjectGenerated && !definition_key.has_value()) return fail("world generated change missing definition");
    return Result<void>::ok();
}

Result<void> CompanionAssistResult::validateBasic() const {
    if (agent_actor_key.empty()) return fail("companion assist agent empty");
    if (summary_zh_cn.empty()) return fail("companion assist summary empty");
    if (containsForbiddenText(summary_zh_cn)) return fail("companion assist forbidden summary");
    return Result<void>::ok();
}

Result<void> WorldInteractionResult::validateBasic() const {
    if (!ok && !failure_kind.has_value()) return fail("world interaction failure kind missing");
    if (ok && failure_kind.has_value()) return fail("world interaction success has failure kind");
    if (player_feedback_zh_cn.empty()) return fail("world interaction player feedback empty");
    if (containsForbiddenText(player_feedback_zh_cn) || containsForbiddenText(trace_keys)) return fail("world interaction forbidden text");
    for (const auto& change : changes) {
        auto result = change.validateBasic();
        if (result.is_error()) return result;
    }
    if (companion_assist.has_value()) {
        auto result = companion_assist->validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> AgentAutonomyRequest::validateBasic() const {
    if (request_key.empty()) return fail("agent autonomy request key empty");
    if (agent_actor_key.empty()) return fail("agent autonomy actor empty");
    if (trigger_key.empty()) return fail("agent autonomy trigger empty");
    return Result<void>::ok();
}

Result<void> AgentAutonomyResult::validateBasic() const {
    if (agent_actor_key.empty()) return fail("agent autonomy result actor empty");
    if (summary_zh_cn.empty()) return fail("agent autonomy result summary empty");
    if (containsForbiddenText(summary_zh_cn)) return fail("agent autonomy forbidden summary");
    for (const auto& change : changes) {
        auto result = change.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

Result<void> WorldObjectProjectionPatch::validateBasic() const {
    if (object_key.empty()) return fail("world object projection key empty");
    if (containsForbiddenText(status_summary_zh_cn) || containsForbiddenText(safe_tags)) return fail("world object projection forbidden text");
    return Result<void>::ok();
}

Result<void> WorldProjectionPatch::validateBasic() const {
    if (containsForbiddenText(scene_summary_zh_cn) || containsForbiddenText(player_feedback_lines_zh_cn)) return fail("world projection forbidden text");
    for (const auto& object_patch : object_patches) {
        auto result = object_patch.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_interaction
