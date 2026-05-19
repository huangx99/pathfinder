#include "pathfinder/agent_reasoning/reasoning_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::agent_reasoning {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

namespace {

Result<void> fail(const std::string& message) {
    return Result<void>::fail(makeError(ErrorCode::validation_failed, message));
}

bool unsafeText(const std::string& value) {
    return value.find("hidden_truth") != std::string::npos ||
           value.find("raw_state") != std::string::npos ||
           value.find("utility_score") != std::string::npos;
}

bool unsafeText(const std::vector<std::string>& values) {
    return std::any_of(values.begin(), values.end(), [](const auto& value) { return unsafeText(value); });
}

bool finiteRange(double value, double min, double max) {
    return std::isfinite(value) && value >= min && value <= max;
}

bool validGoalKind(AgentGoalKind kind, bool allow_test = false) {
    return kind != AgentGoalKind::Unknown && (allow_test || kind != AgentGoalKind::TestOnly);
}

} // namespace

std::string toString(AgentGoalKind kind) {
    switch (kind) {
        case AgentGoalKind::ReduceHunger: return "reduce_hunger";
        case AgentGoalKind::RestoreHealth: return "restore_health";
        case AgentGoalKind::ReduceCold: return "reduce_cold";
        case AgentGoalKind::IncreaseWarmth: return "increase_warmth";
        case AgentGoalKind::IncreaseShelterCapacity: return "increase_shelter_capacity";
        case AgentGoalKind::ReduceThreat: return "reduce_threat";
        case AgentGoalKind::AcquireObject: return "acquire_object";
        case AgentGoalKind::RestoreToolState: return "restore_tool_state";
        case AgentGoalKind::MaintainFire: return "maintain_fire";
        case AgentGoalKind::ProtectDependent: return "protect_dependent";
        case AgentGoalKind::TestOnly: return "test_only";
        case AgentGoalKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(EffectSemanticKind kind) {
    switch (kind) {
        case EffectSemanticKind::NeedDelta: return "need_delta";
        case EffectSemanticKind::ConditionDelta: return "condition_delta";
        case EffectSemanticKind::ThreatDelta: return "threat_delta";
        case EffectSemanticKind::ObjectQuantityDelta: return "object_quantity_delta";
        case EffectSemanticKind::ObjectStateDelta: return "object_state_delta";
        case EffectSemanticKind::CapabilityDelta: return "capability_delta";
        case EffectSemanticKind::KnowledgeDelta: return "knowledge_delta";
        case EffectSemanticKind::LocationSafetyDelta: return "location_safety_delta";
        case EffectSemanticKind::TimeCost: return "time_cost";
        case EffectSemanticKind::RiskDelta: return "risk_delta";
        case EffectSemanticKind::TestOnly: return "test_only";
        case EffectSemanticKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(PlanStepKind kind) {
    switch (kind) {
        case PlanStepKind::DirectAction: return "direct_action";
        case PlanStepKind::PrepareObject: return "prepare_object";
        case PlanStepKind::RestoreTool: return "restore_tool";
        case PlanStepKind::BuildStructure: return "build_structure";
        case PlanStepKind::TeachKnowledge: return "teach_knowledge";
        case PlanStepKind::WaitForCondition: return "wait_for_condition";
        case PlanStepKind::Abort: return "abort";
        case PlanStepKind::TestOnly: return "test_only";
        case PlanStepKind::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(PlanSelectionReason reason) {
    switch (reason) {
        case PlanSelectionReason::HighestUtility: return "highest_utility";
        case PlanSelectionReason::ShortestChain: return "shortest_chain";
        case PlanSelectionReason::LowestRisk: return "lowest_risk";
        case PlanSelectionReason::OnlyExecutable: return "only_executable";
        case PlanSelectionReason::EmergencyOverride: return "emergency_override";
        case PlanSelectionReason::InsufficientKnowledge: return "insufficient_knowledge";
        case PlanSelectionReason::BlockedByResource: return "blocked_by_resource";
        case PlanSelectionReason::BlockedByCondition: return "blocked_by_condition";
        case PlanSelectionReason::CycleDetected: return "cycle_detected";
        case PlanSelectionReason::SearchLimitReached: return "search_limit_reached";
        case PlanSelectionReason::TestOnly: return "test_only";
        case PlanSelectionReason::Unknown: return "unknown";
    }
    return "unknown";
}

std::string toString(KnowledgeUsability usability) {
    switch (usability) {
        case KnowledgeUsability::Usable: return "usable";
        case KnowledgeUsability::Tentative: return "tentative";
        case KnowledgeUsability::Conflicted: return "conflicted";
        case KnowledgeUsability::Dangerous: return "dangerous";
        case KnowledgeUsability::UnknownEffect: return "unknown_effect";
        case KnowledgeUsability::NotOwnedByAgent: return "not_owned_by_agent";
        case KnowledgeUsability::TestOnly: return "test_only";
        case KnowledgeUsability::Unknown: return "unknown";
    }
    return "unknown";
}

Result<void> SemanticDelta::validateBasic() const {
    if (domain.empty() || key.empty() || op.empty() || beneficial_when.empty()) return fail("SemanticDelta required field empty");
    if (unsafeText(domain) || unsafeText(key) || unsafeText(op) || unsafeText(beneficial_when)) return fail("SemanticDelta contains forbidden text");
    if (op != "add" && op != "set" && op != "multiply") return fail("SemanticDelta op unsupported");
    if (beneficial_when != "lower" && beneficial_when != "higher" && beneficial_when != "exists" && beneficial_when != "none") return fail("SemanticDelta beneficial_when unsupported");
    if (!std::isfinite(value)) return fail("SemanticDelta value must be finite");
    return Result<void>::ok();
}

Result<void> EffectSemantics::validateBasic() const {
    if (effect_key.empty() || display_zh_cn.empty() || target_scope.empty()) return fail("EffectSemantics required field empty");
    if (semantic_kind == EffectSemanticKind::Unknown) return fail("EffectSemantics semantic kind unknown");
    if (unsafeText(effect_key) || unsafeText(display_zh_cn) || unsafeText(target_scope) || (required_target_kind && unsafeText(*required_target_kind))) return fail("EffectSemantics contains forbidden text");
    if (goal_affinities.empty() && effect_key != "no_visible_effect" && effect_key != "poison") return fail("EffectSemantics goal affinities empty");
    for (const auto goal : goal_affinities) {
        if (!validGoalKind(goal)) return fail("EffectSemantics goal affinity invalid");
    }
    for (const auto& delta : state_deltas) {
        auto valid = delta.validateBasic();
        if (valid.is_error()) return valid;
    }
    if (!finiteRange(risk_score, 0.0, 100.0)) return fail("EffectSemantics risk_score out of range");
    if (time_cost > 1000) return fail("EffectSemantics time_cost too high");
    if (confidence_floor == pathfinder::knowledge::KnowledgeStatus::Unknown) return fail("EffectSemantics confidence floor unknown");
    return Result<void>::ok();
}

Result<void> AgentNeedState::validateBasic() const {
    if (actor_key.empty()) return fail("AgentNeedState actor key empty");
    if (!finiteRange(hunger, 0.0, 100.0) || !finiteRange(cold, 0.0, 100.0) || !finiteRange(health, 0.0, 100.0) ||
        !finiteRange(fear, 0.0, 100.0) || !finiteRange(fatigue, 0.0, 100.0) || !finiteRange(dependent_pressure, 0.0, 100.0)) {
        return fail("AgentNeedState need values must be 0..100");
    }
    return Result<void>::ok();
}

Result<void> AgentGoal::validateBasic() const {
    if (goal_id.empty() || actor_key.empty() || source_keys.empty()) return fail("AgentGoal required field empty");
    if (!validGoalKind(kind)) return fail("AgentGoal kind invalid");
    if (!finiteRange(urgency, 0.0, 100.0) || !std::isfinite(desired_delta)) return fail("AgentGoal numeric invalid");
    if (unsafeText(goal_id) || unsafeText(actor_key) || unsafeText(source_keys) || (target_key && unsafeText(*target_key))) return fail("AgentGoal contains forbidden text");
    return Result<void>::ok();
}

Result<void> PlanPrecondition::validateBasic() const {
    if (condition_id.empty() || condition_expression.empty() || missing_domain.empty() || missing_key.empty() || required_value.empty()) return fail("PlanPrecondition required field empty");
    if (unsafeText(condition_id) || unsafeText(condition_expression) || unsafeText(missing_domain) || unsafeText(missing_key) || unsafeText(required_value)) return fail("PlanPrecondition contains forbidden text");
    return Result<void>::ok();
}

Result<void> ActionCandidate::validateBasic() const {
    if (candidate_id.empty() || actor_key.empty() || source_knowledge_id.empty() || object_key.empty() || action_key.empty() || effect_key.empty()) return fail("ActionCandidate required field empty");
    if (knowledge_usability == KnowledgeUsability::Unknown) return fail("ActionCandidate usability unknown");
    if (!finiteRange(knowledge_confidence, 0.0, 1.0)) return fail("ActionCandidate knowledge_confidence out of range");
    auto semantics_valid = semantics.validateBasic();
    if (semantics_valid.is_error()) return semantics_valid;
    for (const auto& precondition : blocking_conditions) {
        auto valid = precondition.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> AgentPlanStep::validateBasic() const {
    if (step_id.empty() || actor_key.empty() || object_key.empty() || action_key.empty() || effect_key.empty() || explanation_zh_cn.empty()) return fail("AgentPlanStep required field empty");
    if (kind == PlanStepKind::Unknown) return fail("AgentPlanStep kind unknown");
    if (!finiteRange(risk_score, 0.0, 100.0)) return fail("AgentPlanStep risk out of range");
    if (estimated_ticks > 1000) return fail("AgentPlanStep estimated_ticks too high");
    if (unsafeText(explanation_zh_cn)) return fail("AgentPlanStep explanation forbidden");
    for (const auto& semantics : expected_semantics) {
        auto valid = semantics.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& precondition : preconditions) {
        auto valid = precondition.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> AgentPlan::validateBasic() const {
    if (plan_id.empty() || actor_key.empty()) return fail("AgentPlan required field empty");
    auto goal_valid = goal.validateBasic();
    if (goal_valid.is_error()) return goal_valid;
    if (steps.empty()) return fail("AgentPlan steps empty");
    if (selection_reason == PlanSelectionReason::Unknown) return fail("AgentPlan selection reason unknown");
    if (!std::isfinite(utility_score) || !finiteRange(total_risk_score, 0.0, 600.0)) return fail("AgentPlan score invalid");
    if (unsafeText(plan_id) || unsafeText(actor_key) || unsafeText(trace_keys) || (blocking_reason_zh_cn && unsafeText(*blocking_reason_zh_cn))) return fail("AgentPlan contains forbidden text");
    for (const auto& step : steps) {
        auto valid = step.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> ReasoningOptions::validateBasic() const {
    if (max_depth == 0 || max_depth > 16) return fail("ReasoningOptions max_depth invalid");
    if (max_plan_steps == 0 || max_plan_steps > 32) return fail("ReasoningOptions max_plan_steps invalid");
    if (max_candidates_per_goal == 0 || max_candidates_per_goal > 64) return fail("ReasoningOptions max_candidates_per_goal invalid");
    if (max_total_expansions == 0 || max_total_expansions > 1000) return fail("ReasoningOptions max_total_expansions invalid");
    if (!finiteRange(min_confidence_score, 0.0, 1.0)) return fail("ReasoningOptions min_confidence_score invalid");
    return Result<void>::ok();
}

Result<void> ReasoningRequest::validateBasic() const {
    if (request_id.empty() || actor_key.empty() || trigger_key.empty()) return fail("ReasoningRequest required field empty");
    auto snapshot_valid = world_snapshot.validateBasic();
    if (snapshot_valid.is_error()) return Result<void>::fail(snapshot_valid.errors());
    auto need_valid = need_state.validateBasic();
    if (need_valid.is_error()) return need_valid;
    auto options_valid = options.validateBasic();
    if (options_valid.is_error()) return options_valid;
    if (need_state.actor_key != actor_key) return fail("ReasoningRequest actor mismatch");
    return Result<void>::ok();
}

Result<void> ReasoningTrace::validateBasic() const {
    if (trace_id.empty()) return fail("ReasoningTrace trace_id empty");
    if (unsafeText(trace_id) || unsafeText(public_reason_keys) || unsafeText(debug_reason_keys) || (selected_plan_id && unsafeText(*selected_plan_id))) return fail("ReasoningTrace contains forbidden text");
    return Result<void>::ok();
}

Result<void> ReasoningResult::validateBasic() const {
    auto trace_valid = trace.validateBasic();
    if (trace_valid.is_error()) return trace_valid;
    if (safe_explanation_zh_cn.empty()) return fail("ReasoningResult explanation empty");
    if (unsafeText(safe_explanation_zh_cn)) return fail("ReasoningResult explanation forbidden");
    if (selected_plan) {
        auto valid = selected_plan->validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& goal : goals) {
        auto valid = goal.validateBasic();
        if (valid.is_error()) return valid;
    }
    for (const auto& plan : rejected_plans) {
        auto valid = plan.validateBasic();
        if (valid.is_error()) return valid;
    }
    return Result<void>::ok();
}

Result<void> ReasoningProjection::validateBasic() const {
    if (unsafeText(current_goal_zh_cn) || unsafeText(next_step_zh_cn) || unsafeText(blocked_reason_zh_cn) || unsafeText(public_reason_lines_zh_cn)) return fail("ReasoningProjection contains forbidden text");
    return Result<void>::ok();
}

} // namespace pathfinder::agent_reasoning
