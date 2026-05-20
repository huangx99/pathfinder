#include "pathfinder/world_interaction/world_services.h"
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/agent_reasoning/agent_reasoner.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/goal_execution/goal_execution_system.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace pathfinder::world_interaction {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

namespace {

Result<WorldSnapshot> failSnapshot(const std::string& message) {
    return Result<WorldSnapshot>::fail(makeError(ErrorCode::validation_failed, message));
}

Result<WorldInteractionResult> failInteraction(const std::string& message) {
    return Result<WorldInteractionResult>::fail(makeError(ErrorCode::validation_failed, message));
}

InteractionFailureKind interactionFailureFromExecution(pathfinder::agent_reasoning::ExecutionFailureKind kind) {
    using pathfinder::agent_reasoning::ExecutionFailureKind;
    switch (kind) {
        case ExecutionFailureKind::MissingObject: return InteractionFailureKind::MissingObject;
        case ExecutionFailureKind::MissingTarget: return InteractionFailureKind::MissingTarget;
        case ExecutionFailureKind::InsufficientQuantity: return InteractionFailureKind::InsufficientQuantity;
        case ExecutionFailureKind::ConditionNotMet: return InteractionFailureKind::ConditionNotMet;
        case ExecutionFailureKind::SpecNotFound: return InteractionFailureKind::TargetMismatch;
        case ExecutionFailureKind::ForbiddenBySafety: return InteractionFailureKind::ForbiddenByAudience;
        default: return InteractionFailureKind::ConditionNotMet;
    }
}

Result<std::vector<WorldChange>> failChanges(const std::string& message) {
    return Result<std::vector<WorldChange>>::fail(makeError(ErrorCode::validation_failed, message));
}

Result<AgentAutonomyResult> failAgent(const std::string& message) {
    return Result<AgentAutonomyResult>::fail(makeError(ErrorCode::validation_failed, message));
}

WorldObjectInstanceKind kindForObject(const pathfinder::h5_dialog::DialogScenarioObject& object) {
    const auto& tags = object.safe_tags;
    auto has = [&](const std::string& tag) { return std::find(tags.begin(), tags.end(), tag) != tags.end(); };
    if (object.object_key == "beast_shadow") return WorldObjectInstanceKind::ThreatMarker;
    if (has("generated_item")) return WorldObjectInstanceKind::GeneratedInstance;
    if (has("tool") || has("ignition") || has("maintenance")) return WorldObjectInstanceKind::ToolInstance;
    if (has("fruit") || has("leaf")) return WorldObjectInstanceKind::ConsumableInstance;
    if (has("material") || has("fuel")) return WorldObjectInstanceKind::ResourceStack;
    return WorldObjectInstanceKind::ResourceStack;
}

int intState(const pathfinder::h5_dialog::DialogObjectRuntimeState& runtime, const std::string& key, int fallback) {
    auto it = runtime.numeric_states.find(key);
    if (it == runtime.numeric_states.end()) return fallback;
    return static_cast<int>(std::round(it->second));
}

double doubleState(const WorldObjectInstance& object, const std::string& key) {
    auto it = object.numeric_states.find(key);
    if (it == object.numeric_states.end()) return 0.0;
    return it->second;
}

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

void addTag(std::vector<std::string>& tags, const std::string& tag) {
    if (!hasTag(tags, tag)) tags.push_back(tag);
}

void removeTag(std::vector<std::string>& tags, const std::string& tag) {
    tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
}

ThreatEventPhase phaseFromThreatLevel(double level, bool resolved) {
    if (resolved) return ThreatEventPhase::Resolved;
    if (level >= 75.0) return ThreatEventPhase::Confronting;
    if (level >= 50.0) return ThreatEventPhase::Approaching;
    if (level >= 25.0) return ThreatEventPhase::Foreshadowing;
    return ThreatEventPhase::Dormant;
}

const WorldObjectInstance* findObject(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.objects_by_key.find(key);
    if (it == snapshot.objects_by_key.end()) return nullptr;
    return &it->second;
}

std::string displayObjectName(const WorldSnapshot& snapshot, const std::string& key) {
    const auto* object = findObject(snapshot, key);
    return object && !object->display_name_zh_cn.empty() ? object->display_name_zh_cn : key;
}

const WorldThreatRuntimeState* findThreat(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.threats_by_key.find(key);
    if (it == snapshot.threats_by_key.end()) return nullptr;
    return &it->second;
}

bool actorKnowsEffect(const WorldActorRuntimeState& actor, const std::string& effect_key) {
    return std::find(actor.known_effect_keys.begin(), actor.known_effect_keys.end(), effect_key) != actor.known_effect_keys.end();
}

bool actorCanAccessObject(const WorldObjectInstance& object, const std::string& actor_key) {
    if (actor_key.empty()) return false;
    auto quantity_it = object.actor_quantities.find(actor_key);
    if (quantity_it != object.actor_quantities.end() && quantity_it->second > 0) return true;
    const bool explicitly_permitted = std::find(object.permitted_actor_keys.begin(), object.permitted_actor_keys.end(), actor_key) != object.permitted_actor_keys.end();
    if (!object.actor_quantities.empty()) return explicitly_permitted;
    if (object.owner_actor_key == actor_key) return true;
    if (explicitly_permitted) return true;
    return object.owner_actor_key.empty() && object.permitted_actor_keys.empty();
}

int actorOwnedQuantity(const WorldObjectInstance& object, const std::string& actor_key) {
    auto it = object.actor_quantities.find(actor_key);
    if (it == object.actor_quantities.end()) return 0;
    return std::max(0, it->second);
}

WorldSnapshot actorAccessibleSnapshot(const WorldSnapshot& snapshot, const std::string& actor_key) {
    auto scoped = snapshot;
    auto actor_it = snapshot.actors_by_key.find(actor_key);
    for (auto& [key, object] : scoped.objects_by_key) {
        bool accessible = actorCanAccessObject(object, actor_key);
        if (!accessible && actor_it != snapshot.actors_by_key.end()) {
            accessible = std::find(actor_it->second.held_object_keys.begin(), actor_it->second.held_object_keys.end(), key) != actor_it->second.held_object_keys.end();
        }
        if (actorOwnedQuantity(object, actor_key) > 0) {
            object.numeric_states["world_quantity"] = static_cast<double>(object.quantity);
            object.quantity = actorOwnedQuantity(object, actor_key);
            object.numeric_states["quantity"] = static_cast<double>(object.quantity);
            continue;
        }
        if (!accessible && object.quantity > 0) {
            object.numeric_states["world_quantity"] = static_cast<double>(object.quantity);
            object.quantity = 0;
            object.numeric_states["quantity"] = 0.0;
            addTag(object.state_tags, "not_accessible_to_actor");
        }
    }
    return scoped;
}

bool actorHasAccessibleQuantity(const WorldSnapshot& snapshot, const std::string& actor_key, const std::string& object_key, int required_quantity = 1) {
    auto object_it = snapshot.objects_by_key.find(object_key);
    if (object_it != snapshot.objects_by_key.end()) {
        if (actorOwnedQuantity(object_it->second, actor_key) >= required_quantity) return true;
        if (actorCanAccessObject(object_it->second, actor_key) && object_it->second.actor_quantities.empty() && object_it->second.quantity >= required_quantity) return true;
    }
    auto actor_it = snapshot.actors_by_key.find(actor_key);
    if (actor_it == snapshot.actors_by_key.end()) return false;
    return static_cast<int>(std::count(actor_it->second.held_object_keys.begin(), actor_it->second.held_object_keys.end(), object_key)) >= required_quantity;
}

AgentAutonomyActionKind actionKindFromDriver(pathfinder::goal_execution::ActionDriverKind kind) {
    using pathfinder::goal_execution::ActionDriverKind;
    switch (kind) {
        case ActionDriverKind::CounterThreat: return AgentAutonomyActionKind::HoldTorch;
        case ActionDriverKind::ChopWood:
        case ActionDriverKind::Gather:
        case ActionDriverKind::UseObject:
        case ActionDriverKind::BuildStructure: return AgentAutonomyActionKind::GatherMaterial;
        case ActionDriverKind::SharpenTool: return AgentAutonomyActionKind::MaintainTool;
        case ActionDriverKind::MoveTo: return AgentAutonomyActionKind::ApproachTarget;
        default: return AgentAutonomyActionKind::FollowActor;
    }
}

std::string autonomySummaryFromCommand(
    const WorldActorRuntimeState& actor,
    const WorldSnapshot& snapshot,
    const pathfinder::goal_execution::DriverCommand& command) {
    const auto object_name = command.object_key.has_value() ? displayObjectName(snapshot, *command.object_key) : std::string("当前对象");
    const auto target_name = command.target_key.has_value() ? displayObjectName(snapshot, *command.target_key) : std::string{};
    if (command.effect_key == "repel_beast") return actor.display_name_zh_cn + "根据已知经验使用自己可支配的" + object_name + "应对" + target_name + "。";
    if (command.effect_key == "make_torch") return actor.display_name_zh_cn + "发现缺少火把，于是按已知制作流程尝试制作火把。";
    if (command.effect_key == "ignite_fire") return actor.display_name_zh_cn + "发现制作火把还缺火源，于是按已知流程先点燃火源。";
    if (command.effect_key == "cut_wood") return actor.display_name_zh_cn + "发现制作火把还缺处理好的木材，于是先处理木头。";
    if (command.effect_key == "restore_sharpness") return actor.display_name_zh_cn + "发现工具状态不足，于是先维护工具。";
    return actor.display_name_zh_cn + "根据已知经验推进当前目标：" + command.public_summary_zh_cn;
}

Result<AgentAutonomyResult> executePlanThroughP40(
    const WorldSnapshot& actor_scoped_snapshot,
    const WorldSnapshot& display_snapshot,
    const WorldActorRuntimeState& actor,
    const AgentAutonomyRequest& request,
    const pathfinder::agent_reasoning::AgentPlan& plan) {
    pathfinder::goal_execution::ExecutionContext context;
    context.context_id = "execution.autonomy." + request.agent_actor_key + "." + std::to_string(display_snapshot.turn_index);
    context.actor_key = request.agent_actor_key;
    context.capability_tier = pathfinder::goal_execution::AgentCapabilityTier::CognitiveAgent;
    context.known_claims = actor.known_claims;

    pathfinder::goal_execution::ExecutionTickRequest tick_request;
    tick_request.request_id = "execution_tick.autonomy." + request.agent_actor_key + "." + std::to_string(display_snapshot.turn_index);
    tick_request.context = std::move(context);
    tick_request.world_snapshot = actor_scoped_snapshot;
    tick_request.incoming_plan = plan;
    tick_request.elapsed_ticks = 1;
    tick_request.tick = display_snapshot.turn_index;

    pathfinder::goal_execution::GoalExecutionSystem execution_system;
    auto ticked = execution_system.tick(tick_request);
    if (ticked.is_error()) return Result<AgentAutonomyResult>::fail(ticked.errors());

    AgentAutonomyResult result;
    result.agent_actor_key = request.agent_actor_key;
    result.required_knowledge_effect_key = request.required_knowledge_effect_key;
    result.target_key = request.target_threat_key;
    result.action_kind = AgentAutonomyActionKind::None;
    result.skip_reason = InteractionFailureKind::ConditionNotMet;
    result.summary_zh_cn = ticked.value().safe_summary_zh_cn.empty() ? "同伴正在根据目标执行下一步。" : ticked.value().safe_summary_zh_cn;
    result.trace_keys = ticked.value().trace;

    if (ticked.value().commands_to_resolve.empty()) {
        if (!ticked.value().internal_blockers.empty()) {
            result.summary_zh_cn = ticked.value().internal_blockers.front().safe_summary_zh_cn;
            result.skip_reason = InteractionFailureKind::ToolUnavailable;
        } else if (!ticked.value().generated_subgoals.empty()) {
            result.summary_zh_cn = actor.display_name_zh_cn + "发现当前目标还缺少" + ticked.value().generated_subgoals.front().target_key.value_or("前置条件") + "，已转入准备步骤。";
        }
        return Result<AgentAutonomyResult>::ok(std::move(result));
    }

    WorldInteractionService interaction_service;
    WorldChangeApplier applier;
    auto running_snapshot = actor_scoped_snapshot;
    std::vector<WorldChange> changes;
    for (const auto& input : ticked.value().world_changes) {
        auto resolved = interaction_service.resolve(running_snapshot, input, InteractionResolveOptions{});
        if (resolved.is_error()) return Result<AgentAutonomyResult>::fail(resolved.errors());
        if (!resolved.value().ok) {
            result.summary_zh_cn = resolved.value().player_feedback_zh_cn;
            result.skip_reason = resolved.value().failure_kind.value_or(InteractionFailureKind::ConditionNotMet);
            return Result<AgentAutonomyResult>::ok(std::move(result));
        }
        changes.insert(changes.end(), resolved.value().changes.begin(), resolved.value().changes.end());
        auto applied = applier.apply(running_snapshot, resolved.value().changes);
        if (applied.is_ok()) running_snapshot = applied.value();
    }

    const auto& first_command = ticked.value().commands_to_resolve.front();
    result.executed = !changes.empty();
    result.action_kind = actionKindFromDriver(first_command.driver_kind);
    result.used_object_key = first_command.object_key.value_or("");
    result.target_key = first_command.target_key.value_or(request.target_threat_key);
    result.summary_zh_cn = autonomySummaryFromCommand(actor, display_snapshot, first_command);
    result.skip_reason.reset();
    WorldChange action_change;
    action_change.change_id = "world.agent." + request.agent_actor_key + ".p40_action." + std::to_string(display_snapshot.turn_index);
    action_change.kind = WorldChangeKind::AgentActionExecuted;
    action_change.player_summary_zh_cn = result.summary_zh_cn;
    action_change.target_actor_key = request.agent_actor_key;
    result.changes.push_back(std::move(action_change));
    result.changes.insert(result.changes.end(), changes.begin(), changes.end());
    return Result<AgentAutonomyResult>::ok(std::move(result));
}

std::vector<pathfinder::knowledge::KnowledgeClaim> claimsFromActorKnowledge(
    const WorldActorRuntimeState& actor,
    const AgentAutonomyRequest& request) {
    std::vector<pathfinder::knowledge::KnowledgeClaim> claims;
    for (auto claim : actor.known_claims) {
        if (!request.target_threat_key.empty() && claim.predicate.effect_key == request.required_knowledge_effect_key &&
            claim.subject.related_subject_ids.empty()) {
            claim.subject.related_subject_ids.push_back(request.target_threat_key);
        }
        claims.push_back(std::move(claim));
    }
    return claims;
}



std::string statusSummaryForObject(const WorldObjectInstance& object) {
    std::vector<std::string> parts;
    auto add = [&](const std::string& text) { if (!text.empty()) parts.push_back(text); };
    if (object.definition_key == "red_berry" || object.definition_key == "decayed_red_berry" || object.definition_key == "bitter_leaf" || object.definition_key == "dry_grass") {
        add("剩余：" + std::to_string(object.quantity));
        if (object.quantity <= 0) add("已耗尽");
    } else if (object.definition_key == "wood") {
        add("未处理木头：" + std::to_string(object.quantity));
        const auto processed = static_cast<int>(doubleState(object, "processed"));
        if (processed > 0) add("已处理木材：" + std::to_string(processed));
        if (object.quantity <= 0) add("原木已耗尽");
    } else if (object.definition_key == "axe") {
        add("锋利度：" + std::to_string(static_cast<int>(doubleState(object, "sharpness"))));
    } else if (object.definition_key == "torch") {
        const int pioneer = actorOwnedQuantity(object, "pioneer");
        const int companion = actorOwnedQuantity(object, "companion");
        int assigned_total = 0;
        for (const auto& [_, quantity_value] : object.actor_quantities) assigned_total += std::max(0, quantity_value);
        const int public_count = std::max(0, object.quantity - assigned_total);
        if (object.quantity <= 0) add("尚未制作");
        if (pioneer > 0) add("你的火把：" + std::to_string(pioneer));
        if (companion > 0) add("同伴火把：" + std::to_string(companion));
        if (public_count > 0) add("公共火把：" + std::to_string(public_count));
        if (object.quantity > 0 && pioneer == 0 && companion == 0 && public_count == 0) add("已分配火把：" + std::to_string(object.quantity));
    } else if (object.definition_key == "camp_fire") {
        add(object.quantity > 0 ? "状态：已点燃" : "状态：未点燃");
    } else if (object.definition_key == "beast_shadow") {
        auto threat = doubleState(object, "threat_level");
        if (hasTag(object.state_tags, "failed") || hasTag(object.state_tags, "attacked")) add("状态：已经袭击营地");
        else if (hasTag(object.state_tags, "resolved")) {
            add("状态：已退去，正在外围观察");
            const auto flank_waits = static_cast<int>(doubleState(object, "flank_waits"));
            if (flank_waits > 0) add("迂回观察：" + std::to_string(flank_waits));
        }
        else if (threat >= 75.0) add("状态：正在对峙");
        else if (threat >= 50.0) add("状态：正在靠近");
        else if (threat >= 25.0) add("状态：正在观察");
        else add("状态：潜伏未近身");
        if (doubleState(object, "knows_fire_danger") > 0.0) add("野生 Agent：会本能避开火");
    }
    std::string joined;
    for (size_t index = 0; index < parts.size(); ++index) {
        if (index > 0) joined += "；";
        joined += parts[index];
    }
    return joined;
}

WorldChange change(std::string id, WorldChangeKind kind, std::string summary) {
    WorldChange world_change;
    world_change.change_id = std::move(id);
    world_change.kind = kind;
    world_change.player_summary_zh_cn = std::move(summary);
    return world_change;
}

WorldInteractionResult failure(InteractionFailureKind kind, const std::string& summary) {
    WorldInteractionResult result;
    result.ok = false;
    result.failure_kind = kind;
    result.player_feedback_zh_cn = summary;
    result.trace_keys.push_back("world.failure:" + toString(kind));
    return result;
}

void appendLine(std::string& text, const std::string& line) {
    if (line.empty()) return;
    if (!text.empty() && text.back() != '\n') text += "\n";
    text += line;
}

std::string feedbackFromChanges(const std::vector<WorldChange>& changes) {
    std::string text = "世界变化：";
    for (const auto& world_change : changes) {
        appendLine(text, "变化：" + world_change.player_summary_zh_cn);
    }
    return text;
}

void syncQuantityTag(WorldObjectInstance& object) {
    if (object.quantity <= 0) addTag(object.state_tags, "depleted");
    else removeTag(object.state_tags, "depleted");
}

void syncSharpnessTags(WorldObjectInstance& object) {
    auto sharpness = doubleState(object, "sharpness");
    if (sharpness <= 0.0) {
        object.numeric_states["sharpness"] = 0.0;
        removeTag(object.state_tags, "sharp");
        addTag(object.state_tags, "dull");
    } else {
        removeTag(object.state_tags, "dull");
        addTag(object.state_tags, "sharp");
    }
}

} // namespace

Result<WorldSnapshot> WorldSnapshotAdapter::fromDialogSession(
    const pathfinder::h5_dialog::DialogScenario& scenario,
    const pathfinder::h5_dialog::DialogSessionState& state) const {
    WorldSnapshot snapshot;
    snapshot.snapshot_id = "world.snapshot." + state.session_id + "." + std::to_string(state.turn_index);
    snapshot.scenario_key = state.scenario_key.empty() ? scenario.scenario_key : state.scenario_key;
    snapshot.turn_index = state.turn_index;

    for (const auto& object : scenario.objects) {
        auto runtime_it = state.object_runtime_states.find(object.object_key);
        pathfinder::h5_dialog::DialogObjectRuntimeState runtime;
        runtime.object_key = object.object_key;
        runtime.tag_states = object.safe_tags;
        runtime.numeric_states["quantity"] = 1.0;
        if (runtime_it != state.object_runtime_states.end()) runtime = runtime_it->second;

        WorldObjectInstance instance;
        instance.instance_id = "inst." + object.object_key;
        instance.definition_key = object.object_key;
        instance.display_name_zh_cn = object.display_name;
        instance.kind = kindForObject(object);
        instance.owner_actor_key = runtime.owner_actor_key;
        instance.permitted_actor_keys = runtime.permitted_actor_keys;
        for (const auto& [actor_key, quantity_value] : runtime.actor_quantities) {
            instance.actor_quantities[actor_key] = std::max(0, static_cast<int>(std::round(quantity_value)));
        }
        instance.quantity = intState(runtime, "quantity", 1);
        instance.visible = object.visibility != pathfinder::h5_dialog::DialogObjectVisibility::HiddenFromPlayer;
        instance.usable = !object.allowed_actions.empty();
        instance.state_tags = runtime.tag_states;
        instance.numeric_states = runtime.numeric_states;
        syncQuantityTag(instance);
        if (object.object_key == "axe") syncSharpnessTags(instance);
        snapshot.objects_by_key[object.object_key] = std::move(instance);
    }

    WorldActorRuntimeState pioneer;
    pioneer.actor_key = "pioneer";
    pioneer.display_name_zh_cn = "先驱者";
    pioneer.trust = 1.0;
    pioneer.known_claims = state.actor_claims;
    for (const auto& claim : pioneer.known_claims) {
        if (!claim.predicate.effect_key.empty()) pioneer.known_effect_keys.push_back(claim.predicate.effect_key);
    }
    snapshot.actors_by_key[pioneer.actor_key] = pioneer;

    WorldActorRuntimeState companion;
    companion.actor_key = "companion";
    companion.display_name_zh_cn = "同伴";
    companion.trust = 0.8;
    companion.known_claims = state.recipient_claims;
    for (const auto& claim : companion.known_claims) {
        if (!claim.predicate.effect_key.empty()) companion.known_effect_keys.push_back(claim.predicate.effect_key);
    }
    snapshot.actors_by_key[companion.actor_key] = companion;

    WorldActorRuntimeState beast_actor;
    beast_actor.actor_key = "beast_shadow";
    beast_actor.display_name_zh_cn = "靠近的野兽";
    beast_actor.trust = 0.0;
    beast_actor.known_effect_keys.push_back("fire_is_dangerous");
    beast_actor.is_agent_controlled = true;
    snapshot.actors_by_key[beast_actor.actor_key] = beast_actor;

    auto beast_it = snapshot.objects_by_key.find("beast_shadow");
    if (beast_it != snapshot.objects_by_key.end()) {
        const auto& beast_object = beast_it->second;
        WorldThreatRuntimeState threat;
        threat.threat_key = "beast_shadow";
        threat.display_name_zh_cn = "靠近的野兽";
        threat.level = doubleState(beast_object, "threat_level");
        const bool failed = hasTag(beast_object.state_tags, "failed") || hasTag(beast_object.state_tags, "attacked");
        threat.resolved = hasTag(beast_object.state_tags, "resolved");
        threat.active = threat.level > 0.0 && !threat.resolved && !failed;
        threat.phase = failed ? ThreatEventPhase::Failed : phaseFromThreatLevel(threat.level, threat.resolved);
        if (doubleState(beast_object, "observed_fire") > 0.0) threat.observed_object_keys.push_back("fire");
        if (doubleState(beast_object, "knows_fire_danger") > 0.0) threat.instinct_effect_keys.push_back("fire_is_dangerous");
        snapshot.threats_by_key[threat.threat_key] = threat;
    }

    auto validation = snapshot.validateBasic();
    if (validation.is_error()) return Result<WorldSnapshot>::fail(validation.errors());
    return Result<WorldSnapshot>::ok(std::move(snapshot));
}

Result<void> WorldSnapshotAdapter::writeBackToDialogSession(
    const WorldSnapshot& snapshot,
    pathfinder::h5_dialog::DialogSessionState& state) const {
    auto validation = snapshot.validateBasic();
    if (validation.is_error()) return validation;

    for (const auto& [key, object] : snapshot.objects_by_key) {
        auto& runtime = state.object_runtime_states[key];
        runtime.object_key = key;
        runtime.owner_actor_key = object.owner_actor_key;
        runtime.permitted_actor_keys = object.permitted_actor_keys;
        runtime.actor_quantities.clear();
        for (const auto& [actor_key, quantity_value] : object.actor_quantities) {
            runtime.actor_quantities[actor_key] = static_cast<double>(quantity_value);
        }
        runtime.numeric_states = object.numeric_states;
        runtime.numeric_states["quantity"] = static_cast<double>(object.quantity);
        runtime.tag_states = object.state_tags;
    }
    auto threat_it = snapshot.threats_by_key.find("beast_shadow");
    if (threat_it != snapshot.threats_by_key.end()) {
        auto& runtime = state.object_runtime_states["beast_shadow"];
        runtime.object_key = "beast_shadow";
        runtime.numeric_states["threat_level"] = threat_it->second.level;
        runtime.numeric_states["observed_fire"] = !threat_it->second.observed_object_keys.empty() ? 1.0 : runtime.numeric_states["observed_fire"];
        runtime.numeric_states["knows_fire_danger"] = !threat_it->second.instinct_effect_keys.empty() ? 1.0 : runtime.numeric_states["knows_fire_danger"];
        removeTag(runtime.tag_states, "dormant");
        removeTag(runtime.tag_states, "foreshadowing");
        removeTag(runtime.tag_states, "approaching");
        removeTag(runtime.tag_states, "confronting");
        removeTag(runtime.tag_states, "avoiding_fire");
        removeTag(runtime.tag_states, "failed");
        removeTag(runtime.tag_states, "attacked");
        removeTag(runtime.tag_states, "resolved");
        if (threat_it->second.phase == ThreatEventPhase::Dormant) addTag(runtime.tag_states, "dormant");
        if (threat_it->second.phase == ThreatEventPhase::Foreshadowing) addTag(runtime.tag_states, "foreshadowing");
        if (threat_it->second.phase == ThreatEventPhase::Approaching) addTag(runtime.tag_states, "approaching");
        if (threat_it->second.phase == ThreatEventPhase::Confronting) addTag(runtime.tag_states, "confronting");
        if (threat_it->second.phase == ThreatEventPhase::Mitigated) addTag(runtime.tag_states, "avoiding_fire");
        if (threat_it->second.phase == ThreatEventPhase::Failed) {
            addTag(runtime.tag_states, "failed");
            addTag(runtime.tag_states, "attacked");
        }
        if (threat_it->second.resolved || threat_it->second.phase == ThreatEventPhase::Resolved) addTag(runtime.tag_states, "resolved");
    }
    return Result<void>::ok();
}

Result<WorldInteractionResult> WorldInteractionService::resolve(
    const WorldSnapshot& snapshot,
    const WorldInteractionInput& input,
    const InteractionResolveOptions&) const {
    auto snapshot_validation = snapshot.validateBasic();
    if (snapshot_validation.is_error()) return Result<WorldInteractionResult>::fail(snapshot_validation.errors());
    auto input_validation = input.validateBasic();
    if (input_validation.is_error()) return Result<WorldInteractionResult>::fail(input_validation.errors());

    const auto* object = findObject(snapshot, input.object_key);
    if (!object) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::MissingObject, "这里没有这个东西。"));
    if ((input.action == pathfinder::h5_dialog::DialogActionKind::Eat || input.object_key == "wood" || input.object_key == "torch") && object->quantity <= 0) {
        return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::InsufficientQuantity, object->display_name_zh_cn + "已经没有可消耗的数量了。"));
    }

    pathfinder::agent_reasoning::EffectExecutionSpecRegistry specs;
    pathfinder::agent_reasoning::WorldEffectExecutor executor;
    pathfinder::condition::ConditionExpressionEvaluator evaluator;
    pathfinder::agent_reasoning::WorldExecutionRequest request;
    request.request_id = input.interaction_id + ".p41";
    request.actor_key = input.actor_key;
    request.effect_key = input.effect_key;
    request.object_key = input.object_key;
    request.target_object_key = input.target_object_key;
    request.target_threat_key = input.target_object_key;
    auto executed = executor.execute(snapshot, request, specs, evaluator);
    if (executed.is_error()) return Result<WorldInteractionResult>::fail(executed.errors());

    WorldInteractionResult result;
    result.ok = executed.value().ok;
    result.learning_feedback_key = input.feedback_key;
    result.changes = executed.value().changes;
    for (auto& world_change : result.changes) {
        if ((world_change.kind == WorldChangeKind::ObjectGenerated || world_change.kind == WorldChangeKind::ObjectConsumed) && !world_change.target_actor_key.has_value()) {
            world_change.target_actor_key = input.actor_key;
        }
    }
    if (input.action == pathfinder::h5_dialog::DialogActionKind::Eat) {
        for (auto& world_change : result.changes) {
            if (world_change.kind == WorldChangeKind::ObjectConsumed && world_change.target_instance_id == input.object_key) {
                world_change.player_summary_zh_cn = "你消耗了一个" + object->display_name_zh_cn + "，它在这个地方少了。剩余：" + std::to_string(std::max(0, object->quantity - 1)) + "。";
            }
        }
    }
    result.player_feedback_zh_cn = executed.value().ok ? feedbackFromChanges(result.changes) : executed.value().safe_summary_zh_cn;
    result.trace_keys = executed.value().trace_keys;
    result.trace_keys.push_back("world.interaction:" + input.effect_key);
    if (!executed.value().ok) result.failure_kind = interactionFailureFromExecution(executed.value().failure_kind);

    if (result.changes.empty()) {
        result.ok = false;
        if (!result.failure_kind.has_value()) result.failure_kind = InteractionFailureKind::TargetMismatch;
        if (result.player_feedback_zh_cn.empty()) result.player_feedback_zh_cn = "这个动作暂时没有可结算的世界变化。";
    } else if (result.player_feedback_zh_cn.empty()) {
        result.player_feedback_zh_cn = feedbackFromChanges(result.changes);
    }
    auto validation = result.validateBasic();
    if (validation.is_error()) return Result<WorldInteractionResult>::fail(validation.errors());
    return Result<WorldInteractionResult>::ok(std::move(result));
}

Result<WorldSnapshot> WorldChangeApplier::apply(
    const WorldSnapshot& before,
    const std::vector<WorldChange>& changes) const {
    auto validation = before.validateBasic();
    if (validation.is_error()) return Result<WorldSnapshot>::fail(validation.errors());
    auto after = before;
    for (const auto& world_change : changes) {
        auto change_validation = world_change.validateBasic();
        if (change_validation.is_error()) return Result<WorldSnapshot>::fail(change_validation.errors());
        if (world_change.target_instance_id.has_value()) {
            auto object_it = after.objects_by_key.find(*world_change.target_instance_id);
            if (object_it == after.objects_by_key.end()) return failSnapshot("world change target object missing");
            auto& object = object_it->second;
            if (world_change.quantity_delta != 0) {
                object.quantity += world_change.quantity_delta;
                if (object.quantity < 0) return failSnapshot("world object quantity would become negative");
                object.numeric_states["quantity"] = static_cast<double>(object.quantity);
                syncQuantityTag(object);
                if (world_change.kind == WorldChangeKind::ObjectGenerated) object.visible = true;
                if (world_change.target_actor_key.has_value()) {
                    if (world_change.kind == WorldChangeKind::ObjectGenerated && world_change.quantity_delta > 0) {
                        object.actor_quantities[*world_change.target_actor_key] += world_change.quantity_delta;
                        if (object.owner_actor_key.empty()) object.owner_actor_key = *world_change.target_actor_key;
                    } else if (world_change.kind == WorldChangeKind::ObjectConsumed && world_change.quantity_delta < 0) {
                        auto actor_quantity = object.actor_quantities.find(*world_change.target_actor_key);
                        if (actor_quantity != object.actor_quantities.end()) {
                            actor_quantity->second = std::max(0, actor_quantity->second + world_change.quantity_delta);
                            if (actor_quantity->second == 0) object.actor_quantities.erase(actor_quantity);
                        }
                    }
                }
            }
            if (world_change.state_key.has_value()) {
                if (world_change.numeric_set_value.has_value()) object.numeric_states[*world_change.state_key] = *world_change.numeric_set_value;
                if (world_change.numeric_delta.has_value()) object.numeric_states[*world_change.state_key] += *world_change.numeric_delta;
                if (object.numeric_states[*world_change.state_key] < 0.0) object.numeric_states[*world_change.state_key] = 0.0;
            }
            for (const auto& tag : world_change.tag_add) addTag(object.state_tags, tag);
            for (const auto& tag : world_change.tag_remove) removeTag(object.state_tags, tag);
            if (object.definition_key == "axe") syncSharpnessTags(object);
        }
        if (world_change.target_threat_key.has_value()) {
            auto threat_it = after.threats_by_key.find(*world_change.target_threat_key);
            if (threat_it == after.threats_by_key.end()) return failSnapshot("world change target threat missing");
            auto& threat = threat_it->second;
            if (world_change.kind == WorldChangeKind::ThreatResolved) {
                threat.level = 0.0;
                threat.resolved = true;
                threat.active = false;
                threat.phase = ThreatEventPhase::Resolved;
            } else if (std::find(world_change.tag_add.begin(), world_change.tag_add.end(), "failed") != world_change.tag_add.end() ||
                       std::find(world_change.tag_add.begin(), world_change.tag_add.end(), "attacked") != world_change.tag_add.end()) {
                threat.level = world_change.numeric_set_value.value_or(100.0);
                threat.resolved = false;
                threat.active = false;
                threat.phase = ThreatEventPhase::Failed;
            } else if (world_change.numeric_set_value.has_value()) {
                threat.level = std::clamp(*world_change.numeric_set_value, 0.0, 100.0);
                if (threat.level > 0.0) threat.resolved = false;
                threat.active = threat.level > 0.0;
                threat.phase = phaseFromThreatLevel(threat.level, threat.resolved);
            } else if (world_change.numeric_delta.has_value()) {
                threat.level = std::clamp(threat.level + *world_change.numeric_delta, 0.0, 100.0);
                if (threat.level > 0.0) threat.resolved = false;
                threat.active = threat.level > 0.0;
                threat.phase = phaseFromThreatLevel(threat.level, threat.resolved);
            }
            threat.last_change_reason = world_change.player_summary_zh_cn;
            auto object_it = after.objects_by_key.find(*world_change.target_threat_key);
            if (object_it != after.objects_by_key.end()) {
                object_it->second.numeric_states["threat_level"] = threat.level;
                for (const auto& tag : world_change.tag_add) addTag(object_it->second.state_tags, tag);
                for (const auto& tag : world_change.tag_remove) removeTag(object_it->second.state_tags, tag);
                if (!threat.resolved) removeTag(object_it->second.state_tags, "resolved");
                if (threat.resolved) {
                    addTag(object_it->second.state_tags, "resolved");
                    removeTag(object_it->second.state_tags, "flanking_probe");
                    object_it->second.numeric_states["flank_waits"] = 0.0;
                }
                if (threat.phase == ThreatEventPhase::Mitigated) addTag(object_it->second.state_tags, "avoiding_fire");
                if (threat.phase == ThreatEventPhase::Failed) {
                    addTag(object_it->second.state_tags, "failed");
                    addTag(object_it->second.state_tags, "attacked");
                }
            }
        }
    }
    auto after_validation = after.validateBasic();
    if (after_validation.is_error()) return Result<WorldSnapshot>::fail(after_validation.errors());
    return Result<WorldSnapshot>::ok(std::move(after));
}

Result<AgentAutonomyResult> AgentAutonomyService::tryAct(
    const WorldSnapshot& snapshot,
    const AgentAutonomyRequest& request) const {
    auto snapshot_validation = snapshot.validateBasic();
    if (snapshot_validation.is_error()) return Result<AgentAutonomyResult>::fail(snapshot_validation.errors());
    auto request_validation = request.validateBasic();
    if (request_validation.is_error()) return Result<AgentAutonomyResult>::fail(request_validation.errors());

    AgentAutonomyResult result;
    result.agent_actor_key = request.agent_actor_key;
    result.required_knowledge_effect_key = request.required_knowledge_effect_key;
    result.target_key = request.target_threat_key;
    result.action_kind = AgentAutonomyActionKind::None;
    result.summary_zh_cn = "没有可执行的自主行动。";
    result.skip_reason = InteractionFailureKind::ConditionNotMet;

    auto actor_it = snapshot.actors_by_key.find(request.agent_actor_key);
    if (actor_it == snapshot.actors_by_key.end() || !actor_it->second.can_act) {
        result.skip_reason = InteractionFailureKind::CompanionCannotAct;
        result.summary_zh_cn = "这个 Agent 现在无法行动。";
        return Result<AgentAutonomyResult>::ok(result);
    }
    const auto& actor = actor_it->second;
    const auto* threat = findThreat(snapshot, request.target_threat_key.empty() ? "beast_shadow" : request.target_threat_key);

    if (request.agent_actor_key == "companion") {
        if (!threat || threat->level < 25.0 || threat->resolved) {
            result.skip_reason = InteractionFailureKind::ThreatNotActive;
            result.summary_zh_cn = "同伴暂时没有发现需要立刻处理的危险。";
            return Result<AgentAutonomyResult>::ok(result);
        }
        if (actor.trust < 0.5) {
            result.skip_reason = InteractionFailureKind::CompanionCannotAct;
            result.summary_zh_cn = "同伴还不够信任你，没能立刻协助。";
            return Result<AgentAutonomyResult>::ok(result);
        }

        pathfinder::agent_reasoning::ReasoningRequest reasoning_request;
        reasoning_request.request_id = request.request_key.empty() ? "autonomy.companion" : request.request_key;
        reasoning_request.actor_key = request.agent_actor_key;
        auto scoped_snapshot = actorAccessibleSnapshot(snapshot, request.agent_actor_key);
        reasoning_request.world_snapshot = scoped_snapshot;
        reasoning_request.agent_knowledge = claimsFromActorKnowledge(actor, request);
        reasoning_request.options.min_confidence_score = 0.0;
        reasoning_request.options.allow_tentative_knowledge = true;
        reasoning_request.need_state.actor_key = request.agent_actor_key;
        reasoning_request.need_state.fear = std::clamp(threat->level, 0.0, 100.0);
        reasoning_request.trigger_key = request.trigger_key.empty() ? "agent_autonomy" : request.trigger_key;

        pathfinder::agent_reasoning::AgentReasoner reasoner;
        auto reasoning = reasoner.reason(reasoning_request);
        if (reasoning.is_ok() && reasoning.value().selected_plan) {
            auto planned = executePlanThroughP40(scoped_snapshot, snapshot, actor, request, *reasoning.value().selected_plan);
            if (planned.is_ok()) return planned;
        }

        result.skip_reason = InteractionFailureKind::KnowledgeNotKnown;
        if (actorKnowsEffect(actor, request.required_knowledge_effect_key) && !actorHasAccessibleQuantity(snapshot, request.agent_actor_key, "torch")) {
            result.skip_reason = InteractionFailureKind::ToolUnavailable;
            result.summary_zh_cn = "同伴知道火把能应对危险，但他没有被分配或持有可用火把，所以不能凭空使用你的物品。";
        } else {
            result.summary_zh_cn = reasoning.is_ok() ? "同伴暂时无法根据已知经验形成可执行计划。" + reasoning.value().safe_explanation_zh_cn : "同伴还不能根据已知经验形成可执行计划。";
        }
        return Result<AgentAutonomyResult>::ok(result);
    }

    if (request.agent_actor_key == "beast_shadow") {
        const auto* camp_fire = findObject(snapshot, "camp_fire");
        const auto* torch = findObject(snapshot, "torch");
        const auto actor_holds_torch = std::any_of(snapshot.actors_by_key.begin(), snapshot.actors_by_key.end(), [](const auto& entry) {
            return std::find(entry.second.held_object_keys.begin(), entry.second.held_object_keys.end(), "torch") != entry.second.held_object_keys.end();
        });
        const bool fire_visible = (camp_fire && camp_fire->quantity > 0) || (torch && torch->quantity > 0) || actor_holds_torch ||
            std::find(request.observed_object_keys.begin(), request.observed_object_keys.end(), "fire") != request.observed_object_keys.end();
        const bool knows_fire = actorKnowsEffect(actor, "fire_is_dangerous") ||
            (threat && std::find(threat->instinct_effect_keys.begin(), threat->instinct_effect_keys.end(), "fire_is_dangerous") != threat->instinct_effect_keys.end());
        const auto* threat_object = findObject(snapshot, request.target_threat_key.empty() ? "beast_shadow" : request.target_threat_key);
        const bool flanking_probe = threat_object && hasTag(threat_object->state_tags, "flanking_probe");
        if (!threat || threat->resolved || threat->level < 25.0) {
            result.skip_reason = InteractionFailureKind::ThreatNotActive;
            result.summary_zh_cn = "野生生物还没有进入可见威胁阶段。";
            return Result<AgentAutonomyResult>::ok(result);
        }
        if (fire_visible && knows_fire && !flanking_probe) {
            result.action_kind = AgentAutonomyActionKind::AvoidHazard;
            result.executed = true;
            result.used_object_key = "fire";
            result.skip_reason.reset();
            result.summary_zh_cn = "靠近的野兽观察到火光后选择保持距离。它不是消失了，而是基于本能避开危险。";
            auto world_change = change("world.agent.beast.avoid_fire." + std::to_string(snapshot.turn_index), WorldChangeKind::AgentActionExecuted, result.summary_zh_cn);
            world_change.target_actor_key = "beast_shadow";
            world_change.target_threat_key = "beast_shadow";
            result.changes.push_back(world_change);
            auto threat_change = change("world.agent.beast.threat_down." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, "野兽因为火光降低了靠近意图。");
            threat_change.target_threat_key = "beast_shadow";
            threat_change.numeric_delta = -35.0;
            result.changes.push_back(threat_change);
            return Result<AgentAutonomyResult>::ok(std::move(result));
        }
        if (threat->level >= 100.0) {
            result.action_kind = AgentAutonomyActionKind::AttackTarget;
            result.executed = true;
            result.skip_reason.reset();
            result.summary_zh_cn = "靠近的野兽冲入营地，你和同伴被迫逃散。第一夜失败：准备不足，没有火光或协助挡住它。";
            auto world_change = change("world.agent.beast.attack." + std::to_string(snapshot.turn_index), WorldChangeKind::AgentActionExecuted, result.summary_zh_cn);
            world_change.target_actor_key = "beast_shadow";
            world_change.target_threat_key = "beast_shadow";
            result.changes.push_back(world_change);
            auto threat_change = change("world.agent.beast.attack_outcome." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, "野兽已经袭击营地，这一天的生存尝试失败了。");
            threat_change.target_threat_key = "beast_shadow";
            threat_change.numeric_set_value = 100.0;
            threat_change.tag_add = {"failed", "attacked"};
            result.changes.push_back(threat_change);
            return Result<AgentAutonomyResult>::ok(std::move(result));
        }
        result.skip_reason = InteractionFailureKind::KnowledgeNotKnown;
        result.summary_zh_cn = flanking_probe ? "野兽正在迂回试探，普通火光不足以让它立刻退去，需要主动应对。" : "野生生物没有观察到足以避让的危险，仍在继续靠近。";
        return Result<AgentAutonomyResult>::ok(std::move(result));
    }

    return Result<AgentAutonomyResult>::ok(std::move(result));
}

Result<std::vector<WorldChange>> ThreatEventBridge::progressThreats(
    const WorldSnapshot& snapshot,
    const ThreatProgressInput& input) const {
    auto threat = findThreat(snapshot, input.threat_key);
    if (!threat) return failChanges("threat not found");
    if (threat->resolved) {
        const auto* threat_object = findObject(snapshot, input.threat_key);
        const auto current_flank_waits = threat_object ? doubleState(*threat_object, "flank_waits") : 0.0;
        const auto interval = std::max<uint64_t>(1, input.reentry_interval_waits);
        const auto next_flank_waits = current_flank_waits + 1.0;
        if (next_flank_waits < static_cast<double>(interval)) {
            auto wait_change = change("world.threat.flank_wait." + std::to_string(snapshot.turn_index), WorldChangeKind::ObjectStateChanged, "野兽退到树林外徘徊，暂时没有重新靠近。");
            wait_change.target_instance_id = input.threat_key;
            wait_change.state_key = "flank_waits";
            wait_change.numeric_set_value = next_flank_waits;
            return Result<std::vector<WorldChange>>::ok({wait_change});
        }

        auto reset_change = change("world.threat.flank_reset." + std::to_string(snapshot.turn_index), WorldChangeKind::ObjectStateChanged, "野兽在外围观察了几回合，开始寻找新的接近路线。");
        reset_change.target_instance_id = input.threat_key;
        reset_change.state_key = "flank_waits";
        reset_change.numeric_set_value = 0.0;

        auto world_change = change("world.threat.return." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, "野兽没有彻底离开，它绕到树林另一侧重新靠近营地。");
        world_change.target_threat_key = input.threat_key;
        world_change.numeric_set_value = std::clamp(input.reentry_level, 1.0, 100.0);
        world_change.tag_remove.push_back("resolved");
        world_change.tag_add.push_back("approaching");
        world_change.tag_add.push_back("flanking_probe");
        return Result<std::vector<WorldChange>>::ok({reset_change, world_change});
    }
    auto new_level = std::clamp(threat->level + input.level_delta, 0.0, 100.0);
    std::string summary;
    if (new_level >= 75.0) summary = "野兽已经逼近营地边缘，火光或协助行动会变得很重要。";
    else if (new_level >= 50.0) summary = "草丛里的影子更近了，远处传来低吼。";
    else if (new_level >= 25.0) summary = "树林深处传来短促的响动，像是有什么在观察这里。";
    else summary = "树林暂时保持安静。";
    auto world_change = change("world.threat.progress." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, summary);
    world_change.target_threat_key = input.threat_key;
    world_change.numeric_set_value = new_level;
    return Result<std::vector<WorldChange>>::ok({world_change});
}

Result<std::vector<WorldChange>> ThreatEventBridge::applyCountermeasure(
    const WorldSnapshot& snapshot,
    const ThreatCountermeasureInput& input) const {
    auto threat = findThreat(snapshot, input.threat_key);
    if (!threat) return failChanges("threat not found");
    if (threat->resolved) return Result<std::vector<WorldChange>>::ok({});
    auto world_change = change("world.threat.counter." + input.countermeasure_key + "." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, input.summary_zh_cn.empty() ? "危险被暂时压低。" : input.summary_zh_cn);
    world_change.target_threat_key = input.threat_key;
    world_change.numeric_delta = input.level_delta;
    return Result<std::vector<WorldChange>>::ok({world_change});
}

Result<WorldProjectionPatch> WorldProjectionMapper::buildPatch(
    const WorldSnapshot& snapshot,
    const std::vector<WorldChange>& recent_changes,
    const std::vector<AgentAutonomyResult>& agent_results) const {
    WorldProjectionPatch patch;
    for (const auto& world_change : recent_changes) {
        patch.scene_summary_zh_cn.push_back(world_change.player_summary_zh_cn);
        patch.player_feedback_lines_zh_cn.push_back("变化：" + world_change.player_summary_zh_cn);
        patch.trace_keys.push_back("world.change:" + toString(world_change.kind));
    }
    for (const auto& agent_result : agent_results) {
        if (agent_result.summary_zh_cn.empty()) continue;
        patch.scene_summary_zh_cn.push_back(agent_result.summary_zh_cn);
        patch.player_feedback_lines_zh_cn.push_back(std::string(agent_result.executed ? "Agent行动：" : "Agent状态：") + agent_result.summary_zh_cn);
        patch.trace_keys.push_back("world.agent:" + toString(agent_result.action_kind));
    }
    for (const auto& [object_key, object] : snapshot.objects_by_key) {
        WorldObjectProjectionPatch object_patch;
        object_patch.object_key = object_key;
        object_patch.status_summary_zh_cn = statusSummaryForObject(object);
        object_patch.safe_tags = object.state_tags;
        object_patch.visible = object.visible;
        object_patch.usable = object.usable;
        patch.object_patches.push_back(std::move(object_patch));
    }
    auto validation = patch.validateBasic();
    if (validation.is_error()) return Result<WorldProjectionPatch>::fail(validation.errors());
    return Result<WorldProjectionPatch>::ok(std::move(patch));
}

} // namespace pathfinder::world_interaction
