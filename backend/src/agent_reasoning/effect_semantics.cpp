#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <set>

namespace pathfinder::agent_reasoning {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::knowledge::KnowledgeStatus;

namespace {

SemanticDelta delta(std::string domain, std::string key, double value, std::string beneficial_when, std::string op = "add") {
    SemanticDelta item;
    item.domain = std::move(domain);
    item.key = std::move(key);
    item.value = value;
    item.beneficial_when = std::move(beneficial_when);
    item.op = std::move(op);
    return item;
}

EffectSemantics semantics(
    std::string effect_key,
    std::string display,
    EffectSemanticKind kind,
    std::string target_scope,
    std::vector<AgentGoalKind> affinities,
    std::vector<SemanticDelta> deltas,
    double risk,
    uint32_t time,
    KnowledgeStatus floor = KnowledgeStatus::Active,
    std::optional<std::string> target_kind = std::nullopt) {
    EffectSemantics value;
    value.effect_key = std::move(effect_key);
    value.display_zh_cn = std::move(display);
    value.semantic_kind = kind;
    value.target_scope = std::move(target_scope);
    value.goal_affinities = std::move(affinities);
    value.state_deltas = std::move(deltas);
    value.risk_score = risk;
    value.time_cost = time;
    value.confidence_floor = floor;
    value.required_target_kind = std::move(target_kind);
    return value;
}

} // namespace

std::vector<EffectSemantics> builtInEffectSemantics() {
    return {
        semantics("restore_hunger", "缓解饥饿", EffectSemanticKind::NeedDelta, "self", {AgentGoalKind::ReduceHunger}, {delta("need", "hunger", -30.0, "lower")}, 0.0, 1),
        semantics("poison", "中毒或不适", EffectSemanticKind::RiskDelta, "self", {}, {delta("need", "health", -20.0, "higher"), delta("risk", "poison", 50.0, "lower")}, 80.0, 1),
        semantics("no_visible_effect", "无明显效果", EffectSemanticKind::ConditionDelta, "self", {}, {delta("none", "none", 0.0, "none")}, 10.0, 1, KnowledgeStatus::Candidate),
        semantics("cut_wood", "处理木头", EffectSemanticKind::ObjectQuantityDelta, "object", {AgentGoalKind::AcquireObject}, {delta("object", "wood_processed", 1.0, "exists")}, 10.0, 2),
        semantics("restore_sharpness", "打磨斧头", EffectSemanticKind::ObjectStateDelta, "object", {AgentGoalKind::RestoreToolState}, {delta("object_state", "axe.sharpness", 3.0, "higher")}, 0.0, 1),
        semantics("tool_dull", "工具变钝", EffectSemanticKind::ObjectStateDelta, "object", {AgentGoalKind::RestoreToolState}, {delta("object_state", "axe.sharpness", 0.0, "higher", "set")}, 20.0, 1, KnowledgeStatus::Candidate),
        semantics("use_hint", "摸索用途", EffectSemanticKind::ConditionDelta, "object", {AgentGoalKind::AcquireObject}, {delta("none", "none", 0.0, "none")}, 5.0, 1, KnowledgeStatus::Candidate),
        semantics("ignite_fire", "点燃火堆", EffectSemanticKind::ObjectQuantityDelta, "location", {AgentGoalKind::ReduceCold, AgentGoalKind::IncreaseWarmth, AgentGoalKind::MaintainFire}, {delta("object", "camp_fire", 1.0, "exists"), delta("group", "warmth", 30.0, "higher")}, 10.0, 1),
        semantics("make_torch", "制作火把", EffectSemanticKind::ObjectQuantityDelta, "object", {AgentGoalKind::AcquireObject, AgentGoalKind::ReduceThreat}, {delta("object", "torch", 1.0, "exists")}, 5.0, 2),
        semantics("repel_beast", "驱赶野兽", EffectSemanticKind::ThreatDelta, "target_threat", {AgentGoalKind::ReduceThreat}, {delta("threat", "beast", -100.0, "lower")}, 5.0, 1, KnowledgeStatus::Active, "threat"),
        semantics("provide_warmth", "提供温暖", EffectSemanticKind::NeedDelta, "group", {AgentGoalKind::ReduceCold, AgentGoalKind::IncreaseWarmth}, {delta("need", "cold", -40.0, "lower"), delta("group", "warmth", 40.0, "higher")}, 0.0, 1),
        semantics("build_house", "建造房子", EffectSemanticKind::CapabilityDelta, "group", {AgentGoalKind::IncreaseShelterCapacity, AgentGoalKind::IncreaseWarmth}, {delta("object", "house", 1.0, "exists"), delta("capability", "shelter_capacity", 4.0, "higher"), delta("group", "warmth", 50.0, "higher")}, 15.0, 6)
    };
}

EffectSemanticsRegistry::EffectSemanticsRegistry() : EffectSemanticsRegistry(true) {}

EffectSemanticsRegistry::EffectSemanticsRegistry(bool load_builtins) {
    if (load_builtins) {
        for (const auto& item : builtInEffectSemantics()) {
            definitions_by_effect_key_[item.effect_key] = item;
        }
    }
}

Result<void> EffectSemanticsRegistry::registerDefinition(const EffectSemantics& definition) {
    auto valid = definition.validateBasic();
    if (valid.is_error()) return valid;
    if (definitions_by_effect_key_.find(definition.effect_key) != definitions_by_effect_key_.end()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "effect semantics duplicate effect key"));
    definitions_by_effect_key_[definition.effect_key] = definition;
    return Result<void>::ok();
}

Result<EffectSemantics> EffectSemanticsRegistry::findByEffectKey(const std::string& effect_key) const {
    auto it = definitions_by_effect_key_.find(effect_key);
    if (it != definitions_by_effect_key_.end()) return Result<EffectSemantics>::ok(it->second);
    return Result<EffectSemantics>::fail(makeError(ErrorCode::knowledge_not_found, "effect semantics not found"));
}

Result<std::vector<EffectSemantics>> EffectSemanticsRegistry::findByGoalKind(AgentGoalKind goal_kind) const {
    if (goal_kind == AgentGoalKind::Unknown) {
        return Result<std::vector<EffectSemantics>>::fail(makeError(ErrorCode::validation_enum_unknown, "goal kind unknown"));
    }
    std::vector<EffectSemantics> matches;
    for (const auto& [_, item] : definitions_by_effect_key_) {
        if (std::find(item.goal_affinities.begin(), item.goal_affinities.end(), goal_kind) != item.goal_affinities.end()) {
            matches.push_back(item);
        }
    }
    return Result<std::vector<EffectSemantics>>::ok(std::move(matches));
}

Result<void> EffectSemanticsRegistry::validateAll() const {
    std::set<std::string> seen;
    for (const auto& [key, item] : definitions_by_effect_key_) {
        if (!seen.insert(key).second) return Result<void>::fail(makeError(ErrorCode::validation_failed, "effect semantics duplicate effect key"));
        auto valid = item.validateBasic();
        if (valid.is_error()) return valid;
    }
    const std::vector<std::string> required = {"restore_hunger", "poison", "no_visible_effect", "cut_wood", "restore_sharpness", "ignite_fire", "make_torch", "repel_beast", "provide_warmth", "build_house"};
    for (const auto& key : required) {
        if (definitions_by_effect_key_.find(key) == definitions_by_effect_key_.end()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "required effect semantics missing"));
    }
    return Result<void>::ok();
}

} // namespace pathfinder::agent_reasoning
