#include "pathfinder/content/content_runtime_adapter.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/knowledge/knowledge_types.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace pathfinder::content {

using pathfinder::h5_dialog::DialogScenario;
using pathfinder::h5_dialog::DialogScenarioObject;
using pathfinder::h5_dialog::DialogFeedbackTemplate;
using pathfinder::h5_dialog::DialogScenarioActionTemplate;
using pathfinder::h5_dialog::DialogScenarioThreatKnowledgeTemplate;
using pathfinder::h5_dialog::DialogDefaultKnowledgeTemplate;
using pathfinder::h5_dialog::DialogActionKind;
using pathfinder::h5_dialog::DialogObjectVisibility;
using pathfinder::agent_reasoning::AgentGoalKind;
using pathfinder::agent_reasoning::EffectExecutionOpKind;
using pathfinder::agent_reasoning::EffectExecutionOperation;
using pathfinder::agent_reasoning::EffectExecutionSpec;
using pathfinder::agent_reasoning::EffectExecutionTargetKind;
using pathfinder::agent_reasoning::EffectSemanticKind;
using pathfinder::agent_reasoning::EffectSemantics;
using pathfinder::agent_reasoning::ExecutionValueSourceKind;
using pathfinder::agent_reasoning::SemanticDelta;
using pathfinder::foundation::ErrorCode;

namespace {

pathfinder::foundation::Result<EffectSemanticKind> mapSemanticKind(
    const EffectDefinitionContent& effect_def) {
    if (effect_def.semantic_kind == "actor_need_delta") {
        return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::NeedDelta);
    }
    if (effect_def.semantic_kind == "world_state_delta") {
        for (const auto& op : effect_def.operations) {
            if (op.op == "remove_threat") {
                return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::ThreatDelta);
            }
            if (op.op == "add_object_state_number" || op.op == "set_object_state_number") {
                return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::ObjectStateDelta);
            }
            if (op.op == "produce_object_quantity" || op.op == "consume_object_quantity") {
                return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::ObjectQuantityDelta);
            }
        }
        return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::ConditionDelta);
    }
    if (effect_def.semantic_kind == "knowledge_delta") {
        return pathfinder::foundation::Result<EffectSemanticKind>::ok(EffectSemanticKind::KnowledgeDelta);
    }
    return pathfinder::foundation::Result<EffectSemanticKind>::fail(
        pathfinder::foundation::makeError(ErrorCode::validation_enum_unknown,
            "unknown effect semantic kind: " + effect_def.semantic_kind));
}

AgentGoalKind mapGoalKind(const std::string& goal_key) {
    if (goal_key == "satisfy_need" || goal_key == "reduce_hunger") return AgentGoalKind::ReduceHunger;
    if (goal_key == "restore_health") return AgentGoalKind::RestoreHealth;
    if (goal_key == "reduce_cold") return AgentGoalKind::ReduceCold;
    if (goal_key == "increase_warmth") return AgentGoalKind::IncreaseWarmth;
    if (goal_key == "increase_shelter_capacity") return AgentGoalKind::IncreaseShelterCapacity;
    if (goal_key == "reduce_threat") return AgentGoalKind::ReduceThreat;
    if (goal_key == "acquire_object") return AgentGoalKind::AcquireObject;
    if (goal_key == "restore_tool_state") return AgentGoalKind::RestoreToolState;
    if (goal_key == "maintain_fire") return AgentGoalKind::MaintainFire;
    if (goal_key == "protect_dependent") return AgentGoalKind::ProtectDependent;
    return AgentGoalKind::Unknown;
}

std::vector<AgentGoalKind> derivedGoalKinds(const EffectDefinitionContent& effect_def) {
    std::vector<AgentGoalKind> goals;
    for (const auto& goal_key : effect_def.goal_kinds) {
        auto mapped = mapGoalKind(goal_key);
        if (mapped != AgentGoalKind::Unknown) goals.push_back(mapped);
    }
    if (!goals.empty()) return goals;

    const auto& key = effect_def.key.value();
    if (key == "restore_hunger") return {AgentGoalKind::ReduceHunger};
    if (key == "cut_wood" || key == "make_torch" || key == "use_hint") return {AgentGoalKind::AcquireObject};
    if (key == "restore_sharpness") return {AgentGoalKind::RestoreToolState};
    if (key == "ignite_fire") return {AgentGoalKind::ReduceCold, AgentGoalKind::IncreaseWarmth, AgentGoalKind::MaintainFire};
    if (key == "repel_beast" || key == "fire_is_dangerous") return {AgentGoalKind::ReduceThreat};
    return {};
}

std::string mapTargetScope(const std::string& target_kind) {
    if (target_kind == "actor_self") return "self";
    if (target_kind == "target_object") return "object";
    if (target_kind == "target_threat") return "target_threat";
    if (target_kind == "location") return "location";
    return target_kind.empty() ? "object" : target_kind;
}

std::string operationTargetKey(const EffectOperationDto& op) {
    if (op.target == "$object") return "object";
    if (op.target == "$actor") return "actor";
    return op.target;
}

void addSemanticDelta(std::vector<SemanticDelta>& deltas, std::string domain, std::string key,
    double value, std::string beneficial_when, std::string op = "add") {
    SemanticDelta delta;
    delta.domain = std::move(domain);
    delta.key = std::move(key);
    delta.value = value;
    delta.beneficial_when = std::move(beneficial_when);
    delta.op = std::move(op);
    deltas.push_back(std::move(delta));
}

EffectExecutionTargetKind targetKindForOperation(const EffectOperationDto& op) {
    if (op.target == "$object") return EffectExecutionTargetKind::ObjectSelf;
    if (op.target == "$actor") return EffectExecutionTargetKind::ActorSelf;
    if (op.op == "remove_threat") return EffectExecutionTargetKind::ThreatTarget;
    if (op.op == "change_actor_need" || op.op == "instinct_fear_response") return EffectExecutionTargetKind::ActorSelf;
    return EffectExecutionTargetKind::RuntimeKey;
}

ExecutionValueSourceKind keySourceForOperation(const EffectOperationDto& op) {
    if (op.target == "$object") return ExecutionValueSourceKind::RequestObjectKey;
    if (op.target == "$actor") return ExecutionValueSourceKind::RequestActorKey;
    return ExecutionValueSourceKind::Constant;
}

pathfinder::foundation::Result<EffectExecutionOpKind> opKindForOperation(const std::string& op_key) {
    if (op_key == "consume_object_quantity") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::ConsumeObjectQuantity);
    }
    if (op_key == "produce_object_quantity") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::AddObjectQuantity);
    }
    if (op_key == "add_object_state_number") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::AddObjectStateNumber);
    }
    if (op_key == "set_object_state_number") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::SetObjectStateNumber);
    }
    if (op_key == "change_actor_need") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::ChangeActorNeed);
    }
    if (op_key == "remove_threat") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::ResolveThreat);
    }
    if (op_key == "hint_object_use" || op_key == "instinct_fear_response") {
        return pathfinder::foundation::Result<EffectExecutionOpKind>::ok(EffectExecutionOpKind::EmitWorldEvent);
    }
    return pathfinder::foundation::Result<EffectExecutionOpKind>::fail(
        pathfinder::foundation::makeError(ErrorCode::validation_enum_unknown,
            "unknown effect operation op: " + op_key));
}

} // namespace

ContentRuntimeAdapter::ContentRuntimeAdapter(std::shared_ptr<const ContentRegistry> registry)
    : registry_(std::move(registry)) {}

// Compatibility bridge: maps content action keys to legacy DialogActionKind enum.
// This is a temporary mapping for P42 phase 1. New actions MUST NOT be added here.
// Future phases should use ActionDefinition + Command Adapter for unified dispatch.
DialogActionKind ContentRuntimeAdapter::mapActionKind(const std::string& action_key) const {
    if (action_key == "eat") return DialogActionKind::Eat;
    if (action_key == "use") return DialogActionKind::Use;
    if (action_key == "inspect") return DialogActionKind::Inspect;
    if (action_key == "wait") return DialogActionKind::Wait;
    if (action_key == "teach") return DialogActionKind::Teach;
    if (action_key == "observe") return DialogActionKind::Observe;
    return DialogActionKind::Unknown;
}

DialogObjectVisibility ContentRuntimeAdapter::mapVisibility(ContentVisibility vis) const {
    switch (vis) {
        case ContentVisibility::RuntimeVisible:
            return DialogObjectVisibility::Visible;
        case ContentVisibility::HiddenUntilDiscovered:
            return DialogObjectVisibility::HiddenFromPlayer;
        case ContentVisibility::SystemOnly:
            return DialogObjectVisibility::HiddenFromPlayer;
        case ContentVisibility::TestOnly:
            return DialogObjectVisibility::Unknown;
    }
    return DialogObjectVisibility::Unknown;
}

bool ContentRuntimeAdapter::hasScenario(const std::string& scenario_key) const {
    return registry_->findScenario(scenario_key) != nullptr;
}

pathfinder::foundation::Result<DialogScenarioObject> ContentRuntimeAdapter::buildDialogObject(
    const ObjectDefinitionContent& obj_def) const {

    DialogScenarioObject obj;
    obj.object_key = obj_def.key.value();
    obj.display_name = registry_->translate("zh_cn", obj_def.display_key);
    obj.player_description = registry_->translate("zh_cn", obj_def.description_key);
    obj.visibility = mapVisibility(obj_def.visibility);
    obj.initial_quantity = static_cast<double>(obj_def.default_quantity);
    obj.initial_numeric_states = obj_def.default_numeric;

    // Map allowed_actions to DialogActionKind
    for (const auto& action_str : obj_def.allowed_actions) {
        auto kind = mapActionKind(action_str);
        if (kind != DialogActionKind::Unknown) {
            obj.allowed_actions.push_back(kind);
        }
    }

    // Default action is first in list
    if (!obj.allowed_actions.empty()) {
        obj.default_action = obj.allowed_actions[0];
    }

    // Build input aliases from safe_tags and key
    obj.input_aliases.push_back(obj_def.key.value());
    for (const auto& tag : obj_def.safe_tags) {
        obj.input_aliases.push_back(tag);
    }

    // Copy safe tags for projection
    obj.safe_tags = obj_def.safe_trait_keys;

    return pathfinder::foundation::Result<DialogScenarioObject>::ok(std::move(obj));
}

pathfinder::foundation::Result<DialogFeedbackTemplate> ContentRuntimeAdapter::buildDialogFeedback(
    const FeedbackDefinitionContent& fb_def) const {

    DialogFeedbackTemplate tmpl;
    tmpl.feedback_key = fb_def.key.value();
    tmpl.object_key = fb_def.object_key;
    tmpl.target_object_key = fb_def.target_object_key;
    tmpl.action = mapActionKind(fb_def.action_key);
    tmpl.effect_key = fb_def.effect_key;
    tmpl.condition_keys = fb_def.conditions;
    tmpl.utility_delta = fb_def.utility_delta;
    tmpl.risk_delta = fb_def.risk_delta;
    tmpl.reason_keys = fb_def.causal_tags;
    for (const auto& sig_str : fb_def.outcome_signal_keys) {
        auto sig = pathfinder::cognition::cognitionOutcomeSignalFromString(sig_str);
        if (sig.is_ok()) {
            tmpl.outcome_signals.push_back(sig.value());
        }
    }

    return pathfinder::foundation::Result<DialogFeedbackTemplate>::ok(std::move(tmpl));
}

pathfinder::foundation::Result<EffectSemantics> ContentRuntimeAdapter::buildEffectSemantic(
    const EffectDefinitionContent& effect_def) const {

    EffectSemantics semantics;
    semantics.effect_key = effect_def.key.value();
    semantics.display_zh_cn = registry_->translate("zh_cn", effect_def.display_key);
    auto kind = mapSemanticKind(effect_def);
    if (kind.is_error()) return pathfinder::foundation::Result<EffectSemantics>::fail(kind.errors());
    semantics.semantic_kind = kind.value();
    semantics.target_scope = mapTargetScope(effect_def.target_kind);
    semantics.goal_affinities = derivedGoalKinds(effect_def);
    semantics.risk_score = static_cast<double>(effect_def.risk_score) * 10.0;
    semantics.time_cost = static_cast<uint32_t>(std::max(0, effect_def.time_cost));
    semantics.confidence_floor = effect_def.teachable
        ? pathfinder::knowledge::KnowledgeStatus::Active
        : pathfinder::knowledge::KnowledgeStatus::Candidate;
    if (effect_def.target_kind == "target_threat") semantics.required_target_kind = "threat";

    for (const auto& op : effect_def.operations) {
        if (op.op == "consume_object_quantity") {
            if (!op.state_key.empty()) {
                addSemanticDelta(semantics.state_deltas, "object_state", operationTargetKey(op) + "." + op.state_key,
                    -std::abs(static_cast<double>(op.quantity)), "higher");
            } else {
                addSemanticDelta(semantics.state_deltas, "object", operationTargetKey(op),
                    -std::abs(static_cast<double>(op.quantity)), "higher");
            }
        } else if (op.op == "produce_object_quantity") {
            if (!op.state_key.empty()) {
                addSemanticDelta(semantics.state_deltas, "object_state", operationTargetKey(op) + "." + op.state_key,
                    std::abs(static_cast<double>(op.quantity)), "higher");
            } else {
                addSemanticDelta(semantics.state_deltas, "object", operationTargetKey(op),
                    std::abs(static_cast<double>(op.quantity)), "exists");
            }
        } else if (op.op == "change_actor_need") {
            addSemanticDelta(semantics.state_deltas, "need", op.need_key, op.delta,
                op.delta < 0.0 ? "lower" : "higher");
        } else if (op.op == "add_object_state_number") {
            addSemanticDelta(semantics.state_deltas, "object_state", operationTargetKey(op) + "." + op.state_key,
                op.delta, op.delta < 0.0 ? "lower" : "higher");
        } else if (op.op == "set_object_state_number") {
            addSemanticDelta(semantics.state_deltas, "object_state", operationTargetKey(op) + "." + op.state_key,
                op.value, "higher", "set");
        } else if (op.op == "remove_threat") {
            addSemanticDelta(semantics.state_deltas, "threat", operationTargetKey(op), -100.0, "lower");
        } else if (op.op == "hint_object_use" || op.op == "instinct_fear_response") {
            addSemanticDelta(semantics.state_deltas, "none", "none", 0.0, "none");
        }
    }
    if (semantics.state_deltas.empty()) {
        addSemanticDelta(semantics.state_deltas, "none", "none", 0.0, "none");
    }

    auto valid = semantics.validateBasic();
    if (valid.is_error()) return pathfinder::foundation::Result<EffectSemantics>::fail(valid.errors());
    return pathfinder::foundation::Result<EffectSemantics>::ok(std::move(semantics));
}

pathfinder::foundation::Result<EffectExecutionSpec> ContentRuntimeAdapter::buildEffectSpec(
    const EffectDefinitionContent& effect_def) const {

    EffectExecutionSpec spec;
    spec.spec_id = "spec." + effect_def.key.value();
    spec.effect_key = effect_def.key.value();
    spec.allow_agent_use = effect_def.usable_by_ai;
    spec.allow_player_use = true;
    spec.safe_summary_zh_cn = registry_->translate("zh_cn", effect_def.display_key);
    spec.source_config_id = "content.core.effects." + effect_def.key.value();
    spec.version = "1";
    spec.trace_keys = {"p42.content_effect:" + effect_def.key.value()};

    for (const auto& condition_key : effect_def.preconditions) {
        if (condition_key.find("$object") != std::string::npos ||
            condition_key.find("$actor") != std::string::npos) {
            continue;
        }
        pathfinder::condition::ConditionExpressionRef ref;
        ref.inline_canonical_key = condition_key;
        spec.condition_refs.push_back(std::move(ref));
    }

    for (size_t index = 0; index < effect_def.operations.size(); ++index) {
        const auto& src = effect_def.operations[index];
        auto op_kind = opKindForOperation(src.op);
        if (op_kind.is_error()) return pathfinder::foundation::Result<EffectExecutionSpec>::fail(op_kind.errors());

        EffectExecutionOperation dst;
        dst.operation_id = effect_def.key.value() + ".op" + std::to_string(index + 1);
        dst.op_kind = op_kind.value();
        if ((src.op == "consume_object_quantity" || src.op == "produce_object_quantity") && !src.state_key.empty()) {
            dst.op_kind = EffectExecutionOpKind::AddObjectStateNumber;
        }
        dst.target_kind = targetKindForOperation(src);
        dst.key_source = keySourceForOperation(src);
        dst.runtime_key = src.target == "$object" || src.target == "$actor" ? "" : src.target;
        dst.quantity_value = dst.op_kind == EffectExecutionOpKind::AddObjectStateNumber ? 0 : std::abs(src.quantity);
        dst.number_value = src.op == "set_object_state_number" ? src.value : src.delta;
        if (src.op == "consume_object_quantity" && !src.state_key.empty()) {
            dst.number_value = -std::abs(static_cast<double>(src.quantity));
        } else if (src.op == "produce_object_quantity" && !src.state_key.empty()) {
            dst.number_value = std::abs(static_cast<double>(src.quantity));
        }
        dst.state_key = src.op == "change_actor_need" ? src.need_key : src.state_key;
        dst.safe_summary_zh_cn = registry_->translate("zh_cn", src.summary_key);
        if (dst.safe_summary_zh_cn.empty()) dst.safe_summary_zh_cn = spec.safe_summary_zh_cn;
        dst.trace_keys = {"p42.content_operation:" + dst.operation_id};
        spec.operations.push_back(std::move(dst));
    }

    auto valid = spec.validateBasic();
    if (valid.is_error()) return pathfinder::foundation::Result<EffectExecutionSpec>::fail(valid.errors());
    return pathfinder::foundation::Result<EffectExecutionSpec>::ok(std::move(spec));
}

pathfinder::foundation::Result<std::vector<EffectExecutionSpec>> ContentRuntimeAdapter::buildEffectSpecs() const {
    std::vector<EffectExecutionSpec> specs;
    for (const auto* effect : registry_->allEffects()) {
        auto result = buildEffectSpec(*effect);
        if (result.is_error()) return pathfinder::foundation::Result<std::vector<EffectExecutionSpec>>::fail(result.errors());
        specs.push_back(std::move(result.value()));
    }
    std::sort(specs.begin(), specs.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.effect_key < rhs.effect_key;
    });
    return pathfinder::foundation::Result<std::vector<EffectExecutionSpec>>::ok(std::move(specs));
}

pathfinder::foundation::Result<std::vector<EffectSemantics>> ContentRuntimeAdapter::buildEffectSemantics() const {
    std::vector<EffectSemantics> semantics;
    for (const auto* effect : registry_->allEffects()) {
        auto result = buildEffectSemantic(*effect);
        if (result.is_error()) return pathfinder::foundation::Result<std::vector<EffectSemantics>>::fail(result.errors());
        semantics.push_back(std::move(result.value()));
    }
    std::sort(semantics.begin(), semantics.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.effect_key < rhs.effect_key;
    });
    return pathfinder::foundation::Result<std::vector<EffectSemantics>>::ok(std::move(semantics));
}

pathfinder::foundation::Result<DialogScenario> ContentRuntimeAdapter::buildDialogScenario(
    const std::string& scenario_key) const {

    const ScenarioDefinitionContent* scenario_def = registry_->findScenario(scenario_key);
    if (!scenario_def) {
        return pathfinder::foundation::Result<DialogScenario>::fail(
            pathfinder::foundation::makeError(ErrorCode::id_not_found,
                "scenario not found: " + scenario_key));
    }

    DialogScenario scenario;
    scenario.scenario_key = scenario_def->key.value();
    scenario.display_name = registry_->translate("zh_cn", scenario_def->display_key);

    // welcome_text from locale using welcome_text_key, with fallback
    if (!scenario_def->welcome_text_key.empty()) {
        scenario.welcome_text = registry_->translate("zh_cn", scenario_def->welcome_text_key);
    }
    if (scenario.welcome_text.empty()) {
        scenario.welcome_text = registry_->translate("zh_cn", scenario_def->display_key);
    }

    // Build objects from initial_objects
    for (const auto& init_obj : scenario_def->initial_objects) {
        const ObjectDefinitionContent* obj_def = registry_->findObject(init_obj.object_key);
        if (!obj_def) continue;

        auto obj_result = buildDialogObject(*obj_def);
        if (obj_result.is_ok()) {
            auto obj = obj_result.value();
            obj.initial_quantity = static_cast<double>(init_obj.quantity);
            obj.initial_numeric_states = init_obj.numeric;
            scenario.objects.push_back(std::move(obj));
        }
    }

    // Collect all feedbacks referencing objects in this scenario
    std::set<std::string> scenario_object_keys;
    for (const auto& init_obj : scenario_def->initial_objects) {
        scenario_object_keys.insert(init_obj.object_key);
    }

    for (const auto& fb : registry_->allFeedbacks()) {
        if (scenario_object_keys.count(fb->object_key) > 0) {
            auto fb_result = buildDialogFeedback(*fb);
            if (fb_result.is_ok()) {
                scenario.feedbacks.push_back(fb_result.value());
            }
        }
    }

    // quick_action_input_texts from scenario definition
    scenario.quick_action_input_texts = scenario_def->quick_action_input_texts;

    // suggested_action_templates from scenario definition
    for (const auto& act_tmpl : scenario_def->suggested_action_templates) {
        DialogScenarioActionTemplate dst;
        dst.action_key = act_tmpl.action_key;
        dst.label_text = act_tmpl.label_text;
        dst.input_text = act_tmpl.input_text;
        dst.object_key = act_tmpl.object_key;
        dst.required_effect_key = act_tmpl.required_effect_key;
        dst.target_object_key = act_tmpl.target_object_key;
        dst.reason_keys = act_tmpl.reason_keys;
        scenario.suggested_action_templates.push_back(std::move(dst));
    }

    // threat_knowledge_templates from scenario definition
    for (const auto& thr_tmpl : scenario_def->threat_knowledge_templates) {
        DialogScenarioThreatKnowledgeTemplate dst;
        dst.template_key = thr_tmpl.template_key;
        dst.threat_object_key = thr_tmpl.threat_object_key;
        dst.required_effect_key = thr_tmpl.required_effect_key;
        dst.fallback_effect_keys = thr_tmpl.fallback_effect_keys;
        scenario.threat_knowledge_templates.push_back(std::move(dst));
    }

    auto add_knowledge = [&](const std::string& kt_key, const std::string& owner_display_key,
        bool visible_to_player, bool usable_by_ai) {
        const KnowledgeTemplateContent* kt = registry_->findKnowledgeTemplate(kt_key);
        if (!kt) return;
        DialogDefaultKnowledgeTemplate dkt;
        dkt.template_key = kt->key.value();
        dkt.owner_display_key = owner_display_key;
        dkt.subject_object_key = kt->subject_object_key;
        dkt.target_object_key = kt->target_object_key;
        dkt.action_key = kt->action_key;
        dkt.effect_key = kt->effect_key;
        dkt.usable_by_ai = usable_by_ai && kt->usable_by_ai;
        dkt.usable_for_action = true;
        dkt.visible_to_player = visible_to_player;
        scenario.default_knowledge_templates.push_back(std::move(dkt));
    };

    // default_player_knowledge -> default_knowledge_templates
    for (const auto& kt_key : scenario_def->default_player_knowledge) {
        add_knowledge(kt_key, "pioneer", true, false);
    }

    // default_agent_knowledge -> default_knowledge_templates
    for (const auto& kt_key : scenario_def->default_agent_knowledge) {
        add_knowledge(kt_key, "companion", false, true);
    }

    return pathfinder::foundation::Result<DialogScenario>::ok(std::move(scenario));
}

} // namespace pathfinder::content
