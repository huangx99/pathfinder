#include "pathfinder/world_interaction/world_services.h"
#include "pathfinder/agent_reasoning/agent_reasoner.h"
#include "pathfinder/foundation/error.h"
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

const WorldThreatRuntimeState* findThreat(const WorldSnapshot& snapshot, const std::string& key) {
    auto it = snapshot.threats_by_key.find(key);
    if (it == snapshot.threats_by_key.end()) return nullptr;
    return &it->second;
}

bool actorKnowsEffect(const WorldActorRuntimeState& actor, const std::string& effect_key) {
    return std::find(actor.known_effect_keys.begin(), actor.known_effect_keys.end(), effect_key) != actor.known_effect_keys.end();
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
        add(object.quantity > 0 ? "可用火把：" + std::to_string(object.quantity) : "尚未制作");
    } else if (object.definition_key == "camp_fire") {
        add(object.quantity > 0 ? "状态：已点燃" : "状态：未点燃");
    } else if (object.definition_key == "beast_shadow") {
        auto threat = doubleState(object, "threat_level");
        if (hasTag(object.state_tags, "failed") || hasTag(object.state_tags, "attacked")) add("状态：已经袭击营地");
        else if (hasTag(object.state_tags, "resolved")) add("状态：已退去");
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
    companion.held_object_keys.push_back("torch");
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
        return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::InsufficientQuantity, object->display_name_zh_cn + "已经没有可用数量了。"));
    }

    WorldInteractionResult result;
    result.ok = true;
    result.learning_feedback_key = input.feedback_key;
    result.trace_keys.push_back("world.interaction:" + input.effect_key);
    const auto prefix = "world.change." + std::to_string(snapshot.turn_index) + "." + input.effect_key + ".";

    auto add_quantity = [&](const std::string& id, const std::string& key, int delta, const std::string& summary) {
        auto world_change = change(prefix + id, delta < 0 ? WorldChangeKind::ObjectConsumed : WorldChangeKind::ObjectQuantityChanged, summary);
        world_change.target_instance_id = key;
        world_change.quantity_delta = delta;
        world_change.reason_keys.push_back("world.quantity");
        result.changes.push_back(std::move(world_change));
    };
    auto add_numeric_delta = [&](const std::string& id, const std::string& key, const std::string& state_key, double delta, const std::string& summary) {
        auto world_change = change(prefix + id, WorldChangeKind::ObjectStateChanged, summary);
        world_change.target_instance_id = key;
        world_change.state_key = state_key;
        world_change.numeric_delta = delta;
        world_change.reason_keys.push_back("world.state_delta");
        result.changes.push_back(std::move(world_change));
    };
    auto add_numeric_set = [&](const std::string& id, const std::string& key, const std::string& state_key, double value, const std::string& summary) {
        auto world_change = change(prefix + id, WorldChangeKind::ObjectStateChanged, summary);
        world_change.target_instance_id = key;
        world_change.state_key = state_key;
        world_change.numeric_set_value = value;
        world_change.reason_keys.push_back("world.state_set");
        result.changes.push_back(std::move(world_change));
    };

    if (input.action == pathfinder::h5_dialog::DialogActionKind::Eat) {
        add_quantity("consume", input.object_key, -1, "你消耗了一个" + object->display_name_zh_cn + "，它在这个地方少了。剩余：" + std::to_string(std::max(0, object->quantity - 1)) + "。");
        if (input.effect_key == "restore_hunger") {
            auto world_change = change(prefix + "hunger", WorldChangeKind::ActorNeedChanged, "饥饿感缓解了一些。");
            world_change.target_actor_key = input.actor_key;
            result.changes.push_back(std::move(world_change));
        } else if (input.effect_key == "poison") {
            auto world_change = change(prefix + "poison", WorldChangeKind::ActorConditionChanged, "身体状态变差了，这次经验会影响之后的判断。");
            world_change.target_actor_key = input.actor_key;
            result.changes.push_back(std::move(world_change));
        }
    } else if (input.effect_key == "cut_wood") {
        const auto* axe = findObject(snapshot, "axe");
        const auto* wood = findObject(snapshot, "wood");
        if (!axe || !wood) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::MissingObject, "缺少斧头或木头。"));
        if (doubleState(*axe, "sharpness") <= 0.0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::ConditionNotMet, "斧头太钝了，砍不动木头。"));
        if (wood->quantity <= 0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::InsufficientQuantity, "木头已经没有了。"));
        add_quantity("wood", "wood", -1, "木头被斧头砍开，变成了可以继续加工的材料。");
        add_numeric_delta("processed", "wood", "processed", 1.0, "处理好的木材增加了。当前可用于制作火把。");
        add_numeric_delta("sharpness", "axe", "sharpness", -1.0, "斧头的锋利度下降了。当前锋利度：" + std::to_string(static_cast<int>(std::max(0.0, doubleState(*axe, "sharpness") - 1.0))) + "。");
    } else if (input.effect_key == "tool_dull") {
        auto world_change = change(prefix + "dull", WorldChangeKind::ObjectStateChanged, "斧头太钝了，砍不动木头。你需要先打磨它。");
        world_change.target_instance_id = "axe";
        world_change.state_key = "sharpness";
        world_change.numeric_set_value = 0.0;
        result.changes.push_back(std::move(world_change));
    } else if (input.effect_key == "restore_sharpness") {
        add_numeric_set("sharpness", "axe", "sharpness", 3.0, "斧头被磨石重新打磨，锋利度恢复了。");
    } else if (input.effect_key == "ignite_fire") {
        const auto* grass = findObject(snapshot, "dry_grass");
        if (!grass || grass->quantity <= 0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::InsufficientQuantity, "干草已经没有了。"));
        add_quantity("grass", "dry_grass", -1, "干草被火种点燃，一处小火堆出现了。");
        auto generated = change(prefix + "camp_fire", WorldChangeKind::ObjectGenerated, "火光照亮了周围，火堆现在可以被其他生物观察到。");
        generated.target_instance_id = "camp_fire";
        generated.definition_key = "camp_fire";
        generated.quantity_delta = 1;
        result.changes.push_back(std::move(generated));
        add_numeric_set("beast_observed_fire", "beast_shadow", "observed_fire", 1.0, "火光让树林里的影子变得犹豫。野生生物已经观察到火。");
    } else if (input.effect_key == "make_torch") {
        const auto* wood = findObject(snapshot, "wood");
        const auto* fire = findObject(snapshot, "camp_fire");
        if (!wood || doubleState(*wood, "processed") <= 0.0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::ConditionNotMet, "还缺少处理好的木材。先用斧头砍木头。"));
        if (!fire || fire->quantity <= 0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::ToolUnavailable, "还缺少稳定火源。先点燃干草形成火堆。"));
        add_numeric_delta("processed", "wood", "processed", -1.0, "你消耗了一份处理好的木材。");
        auto generated = change(prefix + "torch", WorldChangeKind::ObjectGenerated, "你把处理过的木头和火源组合起来，做出了一支火把。");
        generated.target_instance_id = "torch";
        generated.definition_key = "torch";
        generated.quantity_delta = 1;
        result.changes.push_back(std::move(generated));
    } else if (input.effect_key == "repel_beast") {
        const auto* torch = findObject(snapshot, "torch");
        if (!torch || torch->quantity <= 0) return Result<WorldInteractionResult>::ok(failure(InteractionFailureKind::ToolUnavailable, "火把还没有做出来，不能用它驱赶野兽。"));
        auto threat_change = change(prefix + "repel", WorldChangeKind::ThreatResolved, "火把的光和热逼退了靠近的野兽。树林里的低吼远去了。");
        threat_change.target_threat_key = "beast_shadow";
        threat_change.quantity_delta = 0;
        result.changes.push_back(std::move(threat_change));
    } else if (input.effect_key == "use_hint") {
        auto world_change = change(prefix + "inspect_use", WorldChangeKind::ObjectStateChanged, "你摸索了" + object->display_name_zh_cn + "的用途，它没有立刻改变世界，但这次尝试被记录为可学习经验。");
        world_change.target_instance_id = input.object_key;
        world_change.reason_keys.push_back("world.no_material_change");
        result.changes.push_back(std::move(world_change));
    } else if (input.effect_key == "no_visible_effect") {
        auto world_change = change(prefix + "no_visible", WorldChangeKind::ActorConditionChanged, "暂时没有明显变化，但这次尝试仍然进入经验记录。");
        world_change.target_actor_key = input.actor_key;
        world_change.reason_keys.push_back("world.no_visible_effect");
        result.changes.push_back(std::move(world_change));
    }

    if (result.changes.empty()) {
        result.ok = false;
        result.failure_kind = InteractionFailureKind::TargetMismatch;
        result.player_feedback_zh_cn = "这个动作暂时没有可结算的世界变化。";
    } else {
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
                if (threat.resolved) addTag(object_it->second.state_tags, "resolved");
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
        reasoning_request.world_snapshot = snapshot;
        reasoning_request.agent_knowledge = claimsFromActorKnowledge(actor, request);
        reasoning_request.need_state.actor_key = request.agent_actor_key;
        reasoning_request.need_state.fear = std::clamp(threat->level, 0.0, 100.0);
        reasoning_request.trigger_key = request.trigger_key.empty() ? "agent_autonomy" : request.trigger_key;

        pathfinder::agent_reasoning::AgentReasoner reasoner;
        auto reasoning = reasoner.reason(reasoning_request);
        if (reasoning.is_ok() && reasoning.value().selected_plan) {
            pathfinder::agent_reasoning::AgentPlanExecutorAdapter adapter;
            auto planned = adapter.toAutonomyResult(*reasoning.value().selected_plan, snapshot);
            if (planned.is_ok()) return planned;
        }

        result.skip_reason = InteractionFailureKind::KnowledgeNotKnown;
        result.summary_zh_cn = reasoning.is_ok() ? reasoning.value().safe_explanation_zh_cn : "同伴还不能根据已知经验形成可执行计划。";
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
        if (!threat || threat->resolved || threat->level < 25.0) {
            result.skip_reason = InteractionFailureKind::ThreatNotActive;
            result.summary_zh_cn = "野生生物还没有进入可见威胁阶段。";
            return Result<AgentAutonomyResult>::ok(result);
        }
        if (fire_visible && knows_fire) {
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
        result.summary_zh_cn = "野生生物没有观察到足以避让的危险，仍在继续靠近。";
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
        auto world_change = change("world.threat.return." + std::to_string(snapshot.turn_index), WorldChangeKind::ThreatLevelChanged, "野兽没有彻底离开，它绕到树林另一侧继续观察营地。");
        world_change.target_threat_key = input.threat_key;
        world_change.numeric_set_value = 25.0;
        world_change.tag_remove.push_back("resolved");
        world_change.tag_add.push_back("foreshadowing");
        return Result<std::vector<WorldChange>>::ok({world_change});
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
        if (agent_result.executed) {
            patch.scene_summary_zh_cn.push_back(agent_result.summary_zh_cn);
            patch.player_feedback_lines_zh_cn.push_back("Agent行动：" + agent_result.summary_zh_cn);
            patch.trace_keys.push_back("world.agent:" + toString(agent_result.action_kind));
        }
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
