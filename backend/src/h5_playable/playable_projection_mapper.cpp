#include "pathfinder/h5_playable/playable_projection_mapper.h"
#include "pathfinder/agent_reasoning/effect_execution.h"
#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/world_interaction/world_services.h"
#include "pathfinder/condition/condition_summary.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace pathfinder::h5_playable {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using namespace pathfinder::h5_projection;

namespace {

SafeTextProjection text(const std::string& key, SafeTextKind kind, const std::string& zh_cn) {
    return makeSafeText(key, kind, zh_cn);
}

bool hasCanonicalCondition(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& canonical_condition_key) {
    for (const auto& condition : claim.conditions) {
        auto canonical = pathfinder::knowledge::canonicalKnowledgeConditionKey(condition);
        if (canonical.is_ok() && canonical.value() == canonical_condition_key) return true;
    }
    return false;
}

std::string objectNameFromKey(const std::string& key) {
    if (key == "red_berry") return "红果";
    if (key == "decayed_red_berry") return "腐烂红果";
    if (key == "bitter_leaf") return "苦叶";
    if (key == "stone_flake") return "石片";
    if (key == "axe") return "斧头";
    if (key == "wood") return "木头";
    if (key == "whetstone") return "磨石";
    if (key == "dry_grass") return "干草";
    if (key == "fire_seed") return "火种";
    if (key == "camp_fire") return "火堆";
    if (key == "torch") return "火把";
    if (key == "beast_shadow") return "靠近的野兽";
    if (key == "companion") return "同伴";
    if (key == "pioneer") return "先驱者";
    return key;
}

std::string objectNameFromClaim(const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (claim.subject.subject_id == "red_berry" && hasCanonicalCondition(claim, "condition:object_state:eq:decayed")) return "腐烂红果";
    return objectNameFromKey(claim.subject.subject_id);
}


std::string targetNameFromClaim(const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (claim.subject.related_subject_ids.empty()) return "";
    return objectNameFromKey(claim.subject.related_subject_ids.front());
}

std::string actionName(const std::string& action_key) {
    if (action_key == "eat") return "吃";
    if (action_key == "use") return "使用";
    if (action_key == "observe") return "观察";
    if (action_key == "teach") return "传授";
    if (action_key == "wait") return "等待";
    return action_key;
}

std::string effectName(const std::string& effect_key) {
    pathfinder::agent_reasoning::EffectSemanticsRegistry registry;
    auto semantics = registry.findByEffectKey(effect_key);
    if (semantics.is_ok() && !semantics.value().display_zh_cn.empty()) return semantics.value().display_zh_cn;
    pathfinder::agent_reasoning::EffectExecutionSpecRegistry specs;
    auto spec = specs.findByEffectKey(effect_key);
    if (spec.is_ok() && !spec.value().safe_summary_zh_cn.empty()) return spec.value().safe_summary_zh_cn;
    return effect_key;
}

bool isRiskEffect(const std::string& effect_key) {
    pathfinder::agent_reasoning::EffectSemanticsRegistry registry;
    auto semantics = registry.findByEffectKey(effect_key);
    return semantics.is_ok() && (semantics.value().semantic_kind == pathfinder::agent_reasoning::EffectSemanticKind::RiskDelta || semantics.value().risk_score >= 70.0);
}

const pathfinder::h5_dialog::DialogObjectRuntimeState* runtimeForObject(
    const pathfinder::h5_dialog::DialogSessionState& state,
    const std::string& object_key) {
    auto it = state.object_runtime_states.find(object_key);
    if (it == state.object_runtime_states.end()) return nullptr;
    return &it->second;
}

double runtimeNumber(const pathfinder::h5_dialog::DialogSessionState& state,
                     const std::string& object_key,
                     const std::string& key) {
    const auto* runtime = runtimeForObject(state, object_key);
    if (!runtime) return 0.0;
    auto it = runtime->numeric_states.find(key);
    if (it == runtime->numeric_states.end()) return 0.0;
    return it->second;
}

bool runtimeHasTag(const pathfinder::h5_dialog::DialogSessionState& state,
                   const std::string& object_key,
                   const std::string& tag) {
    const auto* runtime = runtimeForObject(state, object_key);
    return runtime && std::find(runtime->tag_states.begin(), runtime->tag_states.end(), tag) != runtime->tag_states.end();
}

std::string objectRuntimeStatus(const pathfinder::h5_dialog::DialogSessionState& state,
                                const std::string& object_key) {
    std::vector<std::string> parts;
    auto addQuantity = [&](const std::string& label) {
        parts.push_back(label + std::to_string(static_cast<int>(runtimeNumber(state, object_key, "quantity"))));
    };

    if (object_key == "red_berry" || object_key == "decayed_red_berry" || object_key == "bitter_leaf" || object_key == "dry_grass") {
        addQuantity("剩余：");
        if (runtimeHasTag(state, object_key, "depleted")) parts.push_back("已耗尽");
    } else if (object_key == "wood") {
        addQuantity("未处理木头：");
        const auto processed = static_cast<int>(runtimeNumber(state, object_key, "processed"));
        if (processed > 0) parts.push_back("已处理木材：" + std::to_string(processed));
        if (runtimeHasTag(state, object_key, "depleted")) parts.push_back("原木已耗尽");
    } else if (object_key == "axe") {
        parts.push_back("锋利度：" + std::to_string(static_cast<int>(runtimeNumber(state, object_key, "sharpness"))));
    } else if (object_key == "torch") {
        const auto quantity = static_cast<int>(runtimeNumber(state, object_key, "quantity"));
        parts.push_back(quantity > 0 ? "可用火把：" + std::to_string(quantity) : "尚未制作");
    } else if (object_key == "camp_fire") {
        parts.push_back(runtimeNumber(state, object_key, "quantity") > 0.0 ? "状态：已点燃" : "状态：未点燃");
    } else if (object_key == "beast_shadow") {
        const auto threat = runtimeNumber(state, object_key, "threat_level");
        if (runtimeHasTag(state, object_key, "resolved")) parts.push_back("状态：已退去");
        else if (threat >= 75.0) parts.push_back("状态：正在对峙");
        else if (threat >= 50.0) parts.push_back("状态：正在靠近");
        else if (threat >= 25.0) parts.push_back("状态：正在观察");
        else parts.push_back("状态：潜伏未近身");
        if (runtimeNumber(state, object_key, "knows_fire_danger") > 0.0) parts.push_back("野生 Agent：会本能避开火");
    }

    std::string joined;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) joined += "；";
        joined += parts[i];
    }
    return joined;
}

std::string joinDisplayNames(const std::vector<pathfinder::h5_dialog::DialogScenarioObject>& objects) {
    std::vector<std::string> names;
    for (const auto& object : objects) {
        if (object.visibility == pathfinder::h5_dialog::DialogObjectVisibility::Visible ||
            object.visibility == pathfinder::h5_dialog::DialogObjectVisibility::Mentioned) {
            names.push_back(object.display_name);
        }
    }
    std::string joined;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) joined += "、";
        joined += names[i];
    }
    return joined;
}

bool isThreatTargetEffect(const std::string& effect_key) {
    pathfinder::agent_reasoning::EffectSemanticsRegistry registry;
    auto semantics = registry.findByEffectKey(effect_key);
    return semantics.is_ok() && semantics.value().required_target_kind && *semantics.value().required_target_kind == "threat";
}

bool hasDefaultCompanionThreatCounterKnowledge(const pathfinder::h5_dialog::DialogSessionState& state) {
    return std::any_of(
        state.recipient_claims.begin(),
        state.recipient_claims.end(),
        [](const pathfinder::knowledge::KnowledgeClaim& claim) {
            return claim.predicate.action_key == "use" && isThreatTargetEffect(claim.predicate.effect_key) &&
                   std::find(
                       claim.subject.related_subject_ids.begin(),
                       claim.subject.related_subject_ids.end(),
                       "beast_shadow") != claim.subject.related_subject_ids.end();
        });
}

std::string objectSystemHint(const pathfinder::h5_dialog::DialogScenario& scenario) {
    size_t object_count = 0;
    size_t action_count = 0;
    for (const auto& object : scenario.objects) {
        if (object.visibility != pathfinder::h5_dialog::DialogObjectVisibility::Visible &&
            object.visibility != pathfinder::h5_dialog::DialogObjectVisibility::Mentioned) {
            continue;
        }
        object_count += 1;
        action_count += object.allowed_actions.size();
    }
    return "这个区域现在有 " + std::to_string(object_count) +
           " 个可尝试对象、" + std::to_string(action_count) +
           " 种直接行动。你会把每次尝试记成经验：它当时是什么状态、你做了什么、后来发生了什么。以后遇到草药、蘑菇、工具，也会用同样方式学习。";
}

bool claimHasReason(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& reason_key) {
    return std::find(claim.reason_keys.begin(), claim.reason_keys.end(), reason_key) != claim.reason_keys.end();
}

std::string causalReasonText(const pathfinder::knowledge::KnowledgeClaim& claim) {
    if (claimHasReason(claim, "causal_ambiguous_dose_window")) {
        return "还不能下结论：你刚刚连续尝试过相似东西，结果可能来自其中某一次，也可能来自连续尝试。等一会儿，再单独试一次会更清楚。";
    }
    if (claimHasReason(claim, "causal_confirmation_capped_by_alternative_explanation")) {
        return "还不能下结论：还有别的可能原因没排除，先把它当成经验记录。";
    }
    if (hasCanonicalCondition(claim, "condition:object_state:eq:fresh")) {
        return "这条经验只适用于当前状态；状态变了，结果可能也会变。";
    }
    if (hasCanonicalCondition(claim, "condition:object_state:eq:decayed")) {
        return "这条经验只适用于当前状态；不要直接套用到其他状态。";
    }
    return "";
}

std::string statusName(pathfinder::knowledge::KnowledgeStatus status) {
    switch (status) {
        case pathfinder::knowledge::KnowledgeStatus::Candidate: return "候选判断";
        case pathfinder::knowledge::KnowledgeStatus::Hypothesis: return "经验假设";
        case pathfinder::knowledge::KnowledgeStatus::Active: return "已确认";
        case pathfinder::knowledge::KnowledgeStatus::Teachable: return "可传授";
        case pathfinder::knowledge::KnowledgeStatus::Shared: return "已分享";
        case pathfinder::knowledge::KnowledgeStatus::Operational: return "可用于行动";
        case pathfinder::knowledge::KnowledgeStatus::Deprecated: return "旧知识";
        case pathfinder::knowledge::KnowledgeStatus::Disproven: return "已否定";
        case pathfinder::knowledge::KnowledgeStatus::Unknown: return "未知";
        case pathfinder::knowledge::KnowledgeStatus::TestOnly: return "测试";
    }
    return "未知";
}

bool shouldShowClaim(const pathfinder::knowledge::KnowledgeClaim& claim) {
    return claim.status != pathfinder::knowledge::KnowledgeStatus::Deprecated &&
           claim.status != pathfinder::knowledge::KnowledgeStatus::Disproven &&
           claim.status != pathfinder::knowledge::KnowledgeStatus::Unknown &&
           claim.status != pathfinder::knowledge::KnowledgeStatus::TestOnly;
}

ActionAffordanceKind affordanceFromAction(pathfinder::h5_dialog::DialogActionKind action) {
    if (action == pathfinder::h5_dialog::DialogActionKind::Eat) return ActionAffordanceKind::TryEat;
    if (action == pathfinder::h5_dialog::DialogActionKind::Use) return ActionAffordanceKind::TryUse;
    if (action == pathfinder::h5_dialog::DialogActionKind::Inspect) return ActionAffordanceKind::Inspect;
    if (action == pathfinder::h5_dialog::DialogActionKind::Teach) return ActionAffordanceKind::Teach;
    return ActionAffordanceKind::Inspect;
}

std::string inputForObjectAction(const pathfinder::h5_dialog::DialogScenarioObject& object, pathfinder::h5_dialog::DialogActionKind action) {
    const auto name = object.display_name;
    if (action == pathfinder::h5_dialog::DialogActionKind::Eat) return "吃" + name;
    if (action == pathfinder::h5_dialog::DialogActionKind::Use) return "使用" + name;
    if (action == pathfinder::h5_dialog::DialogActionKind::Inspect) return "观察" + name;
    return "观察";
}

SafeTextKind actionTextKind(pathfinder::h5_dialog::DialogActionKind action) {
    if (action == pathfinder::h5_dialog::DialogActionKind::Eat || action == pathfinder::h5_dialog::DialogActionKind::Use) return SafeTextKind::Feedback;
    return SafeTextKind::Hint;
}

ActionProjection objectAction(const pathfinder::h5_dialog::DialogScenarioObject& object, pathfinder::h5_dialog::DialogActionKind action) {
    ActionProjection projection;
    projection.affordance = affordanceFromAction(action);
    projection.action_key = "playable.action." + pathfinder::h5_dialog::toString(action) + "." + object.object_key;
    projection.label = text(projection.action_key + ".label", actionTextKind(action), inputForObjectAction(object, action));
    projection.input_text = inputForObjectAction(object, action);
    projection.enabled = true;
    projection.command_preview_key = "command." + pathfinder::h5_dialog::toString(action);
    projection.target_object_refs.push_back(object.object_key);
    return projection;
}

ActionProjection globalAction(const std::string& key, ActionAffordanceKind affordance, const std::string& label, const std::string& input) {
    ActionProjection projection;
    projection.action_key = key;
    projection.affordance = affordance;
    projection.label = text(key + ".label", SafeTextKind::Feedback, label);
    projection.input_text = input;
    projection.enabled = true;
    projection.command_preview_key = "command." + key;
    return projection;
}

ConditionSummaryProjection disabledReason(const std::string& key, const std::string& summary) {
    ConditionSummaryProjection reason;
    reason.condition_key = key;
    reason.summary_text = text(key + ".summary", SafeTextKind::Hint, summary);
    reason.blocking = true;
    return reason;
}

bool feedbackAvailable(const pathfinder::h5_dialog::DialogSessionState& state,
                       const pathfinder::h5_dialog::DialogFeedbackTemplate& feedback) {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario = catalog.defaultScenario();
    if (scenario.is_error()) return false;
    pathfinder::world_interaction::WorldSnapshotAdapter adapter;
    auto snapshot = adapter.fromDialogSession(scenario.value(), state);
    if (snapshot.is_error()) return false;
    pathfinder::agent_reasoning::EffectExecutionSpecRegistry specs;
    pathfinder::agent_reasoning::WorldEffectExecutor executor;
    pathfinder::condition::ConditionExpressionEvaluator evaluator;
    pathfinder::agent_reasoning::WorldExecutionRequest request;
    request.request_id = "h5.feedback_available." + feedback.feedback_key;
    request.actor_key = "pioneer";
    request.effect_key = feedback.effect_key;
    request.object_key = feedback.object_key;
    request.target_object_key = feedback.target_object_key;
    request.target_threat_key = feedback.target_object_key;
    request.dry_run = true;
    auto result = executor.execute(snapshot.value(), request, specs, evaluator);
    return result.is_ok() && result.value().ok;
}

std::string targetedActionInputText(const pathfinder::h5_dialog::DialogScenarioObject& source,
                                  const pathfinder::h5_dialog::DialogScenarioObject& target,
                                  const pathfinder::h5_dialog::DialogFeedbackTemplate& feedback) {
    if (source.object_key == "axe" && target.object_key == "wood") return "用" + source.display_name + "砍" + target.display_name;
    if (source.object_key == "whetstone" && target.object_key == "axe") return "用" + source.display_name + "打磨" + target.display_name;
    if (source.object_key == "fire_seed" && target.object_key == "dry_grass") return "用" + source.display_name + "点燃" + target.display_name;
    if (source.object_key == "wood" && target.object_key == "fire_seed") return "制作火把";
    if (source.object_key == "torch" && target.object_key == "beast_shadow") return "用" + source.display_name + "驱赶" + target.display_name;
    if (feedback.action == pathfinder::h5_dialog::DialogActionKind::Use) return "用" + source.display_name + "作用于" + target.display_name;
    return actionName(pathfinder::h5_dialog::toString(feedback.action)) + source.display_name + "到" + target.display_name;
}

const pathfinder::h5_dialog::DialogScenarioObject* findObjectByKey(
    const pathfinder::h5_dialog::DialogScenario& scenario,
    const std::string& object_key) {
    for (const auto& object : scenario.objects) {
        if (object.object_key == object_key) return &object;
    }
    return nullptr;
}

ActionProjection targetedActionFromFeedback(const pathfinder::h5_dialog::DialogScenario& scenario,
                                            const pathfinder::h5_dialog::DialogSessionState& state,
                                            const pathfinder::h5_dialog::DialogFeedbackTemplate& feedback) {
    const auto* source = findObjectByKey(scenario, feedback.object_key);
    const auto* target = findObjectByKey(scenario, feedback.target_object_key);
    const auto input = source && target ? targetedActionInputText(*source, *target, feedback) : feedback.feedback_key;
    auto projection = globalAction("playable.action.targeted." + feedback.feedback_key, ActionAffordanceKind::TryUse, input, input);
    projection.target_object_refs = {feedback.object_key, feedback.target_object_key};
    if (!feedbackAvailable(state, feedback)) {
        projection.enabled = false;
        projection.disabled_reason = disabledReason("condition.not_ready." + feedback.feedback_key, "还缺少前置材料或状态，先完成准备步骤。");
    }
    return projection;
}

std::vector<ActionProjection> targetedActionsForObject(const pathfinder::h5_dialog::DialogScenario& scenario,
                                                       const pathfinder::h5_dialog::DialogSessionState& state,
                                                       const std::string& object_key,
                                                       pathfinder::h5_dialog::DialogActionKind action) {
    std::vector<ActionProjection> actions;
    std::vector<std::string> seen;
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.object_key != object_key || feedback.action != action || feedback.target_object_key.empty()) continue;
        const auto key = feedback.object_key + "|" + feedback.target_object_key + "|" + pathfinder::h5_dialog::toString(feedback.action);
        if (std::find(seen.begin(), seen.end(), key) != seen.end()) continue;
        seen.push_back(key);
        actions.push_back(targetedActionFromFeedback(scenario, state, feedback));
    }
    return actions;
}

bool hasDirectFeedback(const pathfinder::h5_dialog::DialogScenario& scenario,
                       const std::string& object_key,
                       pathfinder::h5_dialog::DialogActionKind action) {
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.object_key == object_key &&
            feedback.action == action &&
            feedback.target_object_key.empty()) {
            return true;
        }
    }
    return false;
}

std::vector<ActionProjection> targetedActions(const pathfinder::h5_dialog::DialogScenario& scenario,
                                             const pathfinder::h5_dialog::DialogSessionState& state) {
    std::vector<ActionProjection> actions;
    std::vector<std::string> seen;
    for (const auto& feedback : scenario.feedbacks) {
        if (feedback.target_object_key.empty()) continue;
        const auto key = feedback.object_key + "|" + feedback.target_object_key + "|" + pathfinder::h5_dialog::toString(feedback.action);
        if (std::find(seen.begin(), seen.end(), key) != seen.end()) continue;
        seen.push_back(key);
        actions.push_back(targetedActionFromFeedback(scenario, state, feedback));
    }
    return actions;
}

std::vector<ActionProjection> globalActions() {
    return {
        globalAction("playable.action.observe", ActionAffordanceKind::Inspect, "观察", "观察"),
        globalAction("playable.action.wait", ActionAffordanceKind::Inspect, "等待一会", "等待一会"),
        globalAction("playable.action.teach_companion", ActionAffordanceKind::Teach, "教同伴", "教同伴"),
        globalAction("playable.action.inspect_actor_knowledge", ActionAffordanceKind::Inspect, "查看知识", "查看知识"),
        globalAction("playable.action.inspect_recipient_knowledge", ActionAffordanceKind::Inspect, "查看同伴", "查看同伴"),
        globalAction("playable.action.restart", ActionAffordanceKind::Inspect, "重开", "重开")
    };
}

StatusBadgeProjection badge(const std::string& key, const std::string& label, const std::string& tone, int priority) {
    StatusBadgeProjection projection;
    projection.badge_key = key;
    projection.label = text(key + ".label", SafeTextKind::Hint, label);
    projection.tone_key = tone;
    projection.priority = priority;
    return projection;
}

std::vector<StatusBadgeProjection> badgesForObject(const std::string& object_key, const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    std::vector<StatusBadgeProjection> badges;
    for (const auto& claim : claims) {
        if (!shouldShowClaim(claim)) continue;
        const bool is_decayed_claim = claim.subject.subject_id == "red_berry" && hasCanonicalCondition(claim, "condition:object_state:eq:decayed");
        const bool matches = claim.subject.subject_id == object_key ||
            (object_key == "decayed_red_berry" && is_decayed_claim);
        if (!matches) continue;
        badges.push_back(badge("knowledge.badge." + object_key + "." + claim.predicate.effect_key, statusName(claim.status), "knowledge", 10));
        break;
    }
    return badges;
}

ObjectCardProjection objectCard(const pathfinder::h5_dialog::DialogScenario& scenario,
                                const pathfinder::h5_dialog::DialogScenarioObject& object,
                                const pathfinder::h5_dialog::DialogSessionState& state,
                                const pathfinder::world_interaction::WorldObjectProjectionPatch* world_patch,
                                const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    ObjectCardProjection card;
    card.object_ref_key = object.object_key;
    card.display_name = text("object." + object.object_key + ".name", SafeTextKind::DisplayName, object.display_name);
    auto description = object.player_description;
    const auto status = world_patch && !world_patch->status_summary_zh_cn.empty() ? world_patch->status_summary_zh_cn : objectRuntimeStatus(state, object.object_key);
    if (!status.empty()) description += "\n当前状态：" + status + "。";
    card.description = text("object." + object.object_key + ".description", SafeTextKind::Description, description);
    card.visibility_key = pathfinder::h5_dialog::toString(object.visibility);
    card.safe_tags = object.safe_tags;
    if (world_patch) {
        for (const auto& tag : world_patch->safe_tags) {
            if (std::find(card.safe_tags.begin(), card.safe_tags.end(), tag) == card.safe_tags.end()) card.safe_tags.push_back(tag);
        }
    } else {
        const auto* runtime = runtimeForObject(state, object.object_key);
        if (runtime) {
            for (const auto& tag : runtime->tag_states) {
                if (std::find(card.safe_tags.begin(), card.safe_tags.end(), tag) == card.safe_tags.end()) card.safe_tags.push_back(tag);
            }
        }
    }
    card.knowledge_badges = badgesForObject(object.object_key, claims);
    for (auto action : object.allowed_actions) {
        if (hasDirectFeedback(scenario, object.object_key, action)) {
            auto direct_action = objectAction(object, action);
            if ((action == pathfinder::h5_dialog::DialogActionKind::Eat || object.object_key == "torch") &&
                runtimeNumber(state, object.object_key, "quantity") <= 0.0) {
                direct_action.enabled = false;
                direct_action.disabled_reason = disabledReason("condition.resource_depleted." + object.object_key, "这个对象现在没有可用数量。");
            }
            card.actions.push_back(direct_action);
        }
        auto targeted = targetedActionsForObject(scenario, state, object.object_key, action);
        card.actions.insert(card.actions.end(), targeted.begin(), targeted.end());
    }
    return card;
}

KnowledgeLineProjection knowledgeLine(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& owner_key, const std::string& owner_label) {
    KnowledgeLineProjection line;
    line.knowledge_id = claim.knowledge_id.empty() ?
        "knowledge." + owner_key + "." + claim.subject.subject_id + "." + claim.predicate.action_key + "." + claim.predicate.effect_key :
        claim.knowledge_id.value();
    line.owner_label = text("knowledge.owner." + owner_key, SafeTextKind::DisplayName, owner_label);
    line.subject_label = text("knowledge.subject." + claim.subject.subject_id, SafeTextKind::DisplayName, objectNameFromClaim(claim));
    std::string predicate_label = actionName(claim.predicate.action_key);
    const auto target_name = targetNameFromClaim(claim);
    if (!target_name.empty()) predicate_label = "对" + target_name + predicate_label;
    line.predicate_label = text("knowledge.predicate." + claim.predicate.action_key, SafeTextKind::DisplayName, predicate_label);
    line.effect_summary = text("knowledge.effect." + claim.predicate.effect_key, SafeTextKind::Description, effectName(claim.predicate.effect_key));
    line.status_key = pathfinder::knowledge::toString(claim.status);
    auto confidence_text = statusName(claim.status);
    const auto causal_text = causalReasonText(claim);
    if (!causal_text.empty()) {
        confidence_text += "；" + causal_text;
    }
    line.confidence_label = text("knowledge.confidence." + line.status_key, SafeTextKind::Hint, confidence_text);
    line.reason_keys = claim.reason_keys;
    line.teachable = claim.teaching_profile.teachable || claim.status == pathfinder::knowledge::KnowledgeStatus::Active || claim.status == pathfinder::knowledge::KnowledgeStatus::Teachable;
    return line;
}

std::vector<KnowledgeLineProjection> knowledgeLines(const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims, const std::string& owner_key, const std::string& owner_label) {
    std::vector<KnowledgeLineProjection> lines;
    for (const auto& claim : claims) {
        if (shouldShowClaim(claim)) lines.push_back(knowledgeLine(claim, owner_key, owner_label));
    }
    return lines;
}

bool containsPoisonKnowledge(const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    for (const auto& claim : claims) {
        if (shouldShowClaim(claim) && isRiskEffect(claim.predicate.effect_key)) return true;
    }
    return false;
}

std::vector<DangerHintProjection> dangerHints(const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    std::vector<DangerHintProjection> hints;
    if (!containsPoisonKnowledge(claims)) return hints;
    DangerHintProjection hint;
    hint.danger_key = "danger.known_bad_food";
    hint.severity_label = text("danger.severity.warning", SafeTextKind::Warning, "需要谨慎");
    hint.hint_text = text("danger.known_bad_food.hint", SafeTextKind::Warning, "你已经有经验表明，某些相似食物可能会让身体不适。");
    hint.countermeasure_labels.push_back(text("danger.countermeasure.observe_first", SafeTextKind::Hint, "再次尝试前，先观察或回忆已有知识。"));
    hint.related_object_refs.push_back("decayed_red_berry");
    hints.push_back(std::move(hint));
    return hints;
}

std::vector<ConditionSummaryProjection> conditionHints(const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    std::vector<ConditionSummaryProjection> hints;
    pathfinder::condition::ConditionSummaryBuilder builder;
    for (const auto& claim : claims) {
        if (!shouldShowClaim(claim)) continue;
        for (const auto& condition : claim.conditions) {
            auto canonical = pathfinder::knowledge::canonicalKnowledgeConditionKey(condition);
            if (canonical.is_error()) continue;
            auto summary = builder.summarizeCanonicalKey(canonical.value());
            ConditionSummaryProjection hint;
            hint.condition_key = canonical.value();
            hint.summary_text = text("condition.summary." + canonical.value(), SafeTextKind::Hint, summary.is_ok() ? summary.value().safe_summary_text : "这条知识只在特定情况下成立。");
            hint.satisfied_hint = std::nullopt;
            hint.blocking = false;
            hints.push_back(std::move(hint));
        }
    }
    return hints;
}

std::string firstSafeReplyLine(const std::string& reply) {
    std::istringstream stream(reply);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(stream, line)) {
        if (line.find("你现在知道：") != std::string::npos) break;
        if (line.find("同伴现在知道：") != std::string::npos) break;
        if (line.rfind("- ", 0) == 0) continue;
        if (line.find(" -> ") != std::string::npos) continue;
        if (!line.empty()) lines.push_back(line);
    }
    if (lines.empty()) return "知识已更新，请查看知识面板。";
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) out += "\n";
        out += lines[i];
    }
    return out;
}

std::string depletedObjectReply(const std::string& reply) {
    if (reply.find("苦叶") != std::string::npos && reply.find("没有可用") != std::string::npos) return "苦叶已经没有可消耗的数量了。";
    if (reply.find("红果") != std::string::npos && reply.find("没有可用") != std::string::npos) return "红果已经没有可消耗的数量了。";
    if (reply.find("干草") != std::string::npos && reply.find("没有可用") != std::string::npos) return "干草已经没有可消耗的数量了。";
    return reply;
}

} // namespace

std::string playableSafeReplyText(const pathfinder::h5_dialog::DialogResponseDto& dialog_response) {
    return depletedObjectReply(firstSafeReplyLine(dialog_response.reply_text));
}

Result<H5ProjectionSourceBundle> H5PlayableProjectionMapper::buildSourceBundle(
    const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
    const pathfinder::h5_dialog::DialogSessionState& session_state) const {
    pathfinder::h5_dialog::DialogScenarioCatalog catalog;
    auto scenario_result = catalog.defaultScenario();
    if (scenario_result.is_error()) return Result<H5ProjectionSourceBundle>::fail(scenario_result.errors());
    const auto scenario = scenario_result.value();

    pathfinder::world_interaction::WorldProjectionPatch world_patch;
    {
        pathfinder::world_interaction::WorldSnapshotAdapter adapter;
        pathfinder::world_interaction::WorldProjectionMapper mapper;
        auto snapshot_result = adapter.fromDialogSession(scenario, session_state);
        if (snapshot_result.is_ok()) {
            auto patch_result = mapper.buildPatch(snapshot_result.value(), {}, {});
            if (patch_result.is_ok()) world_patch = patch_result.value();
        }
    }

    H5ProjectionSourceBundle bundle;
    bundle.scene_title = text("scene.p22_minimal.title", SafeTextKind::DisplayName, "林地边缘");
    bundle.scene_summary.push_back(text("scene.p22_minimal.welcome", SafeTextKind::Description, scenario.welcome_text));
    for (const auto& line : dialog_response.state_summary_lines) {
        bundle.scene_summary.push_back(text("scene.summary.line", SafeTextKind::Feedback, line));
    }
    const auto visible_names = joinDisplayNames(scenario.objects);
    if (!visible_names.empty()) {
        bundle.scene_summary.push_back(text("scene.visible_objects", SafeTextKind::Hint, "你能看见：" + visible_names + "。"));
    }
    if (hasDefaultCompanionThreatCounterKnowledge(session_state)) {
        bundle.scene_summary.push_back(text(
            "scene.companion.default_torch_knowledge",
            SafeTextKind::Hint,
            "同伴知道火把可以驱赶靠近的野兽，但开局没有火把；你需要教会或准备火把制作链路，他才能尝试自己准备并使用。"));
    }
    bundle.scene_summary.push_back(text("scene.config_driven_objects", SafeTextKind::Hint, objectSystemHint(scenario)));

    for (const auto& object : scenario.objects) {
        if (object.visibility == pathfinder::h5_dialog::DialogObjectVisibility::Visible ||
            object.visibility == pathfinder::h5_dialog::DialogObjectVisibility::Mentioned) {
            const pathfinder::world_interaction::WorldObjectProjectionPatch* object_patch = nullptr;
            for (const auto& patch : world_patch.object_patches) {
                if (patch.object_key == object.object_key) {
                    object_patch = &patch;
                    break;
                }
            }
            bundle.object_cards.push_back(objectCard(scenario, object, session_state, object_patch, session_state.actor_claims));
        }
    }

    bundle.action_bar = globalActions();
    const auto target_actions = targetedActions(scenario, session_state);
    bundle.action_bar.insert(bundle.action_bar.begin() + 1, target_actions.begin(), target_actions.end());
    bundle.actor_knowledge = knowledgeLines(session_state.actor_claims, "actor", "你");
    bundle.recipient_knowledge = knowledgeLines(session_state.recipient_claims, "recipient", "同伴");
    bundle.condition_hints = conditionHints(session_state.actor_claims);
    bundle.danger_hints = dangerHints(session_state.actor_claims);

    TribeStatusProjection tribe;
    tribe.tribe_ref_key = "tribe.initial_pair";
    tribe.trust_label = text("tribe.trust.companion", SafeTextKind::Hint, "同伴愿意听你讲述经验。");
    tribe.coordination_label = text("tribe.coordination.learning", SafeTextKind::Hint, "你们还处在最早的共同探索阶段。");
    bundle.tribe_status = tribe;

    bundle.warning_keys = dialog_response.warning_keys;
    auto result = bundle.validateBasic();
    if (result.is_error()) return Result<H5ProjectionSourceBundle>::fail(result.errors());
    return Result<H5ProjectionSourceBundle>::ok(std::move(bundle));
}

} // namespace pathfinder::h5_playable
