#include "pathfinder/danger/hazard_resolver.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace pathfinder::danger {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static bool containsValue(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static void appendUnique(std::vector<std::string>& values, const std::string& value) {
    if (!value.empty() && !containsValue(values, value)) values.push_back(value);
}

static std::string claimKey(const pathfinder::knowledge::KnowledgeClaim& claim) {
    return claim.subject.subject_id + ":" + claim.predicate.action_key + ":" + claim.predicate.effect_key;
}

static bool claimMatches(const pathfinder::knowledge::KnowledgeClaim& claim, const std::string& requirement_key, const HazardRule& rule, const ThreatProfile& threat) {
    if (claim.knowledge_id.value() == requirement_key) return true;
    if (claimKey(claim) == requirement_key) return true;
    if (claim.subject.subject_id == requirement_key) return true;
    if (claim.predicate.effect_key == requirement_key) return true;
    if (!rule.knowledge_effect_key.empty() && claim.predicate.effect_key == rule.knowledge_effect_key) return true;
    if (!threat.threat_key.empty() && claim.subject.subject_id == threat.threat_key) return true;
    if (!threat.source_definition_id.empty() && claim.subject.subject_id == threat.source_definition_id.value()) return true;
    return false;
}

static bool objectMatches(const pathfinder::reaction::ReactionRuntimeObject& object, const std::string& key) {
    if (object.object_id.value() == key || object.definition_id.value() == key || object.object_key == key) return true;
    if (containsValue(object.tag_keys, key) || containsValue(object.state_keys, key)) return true;
    return object.resource_values.find(key) != object.resource_values.end();
}

static Result<bool> conditionMatches(
    const pathfinder::condition::ConditionExpressionRef& ref,
    const HazardExposureInput& input,
    const ThreatProfile& threat,
    const pathfinder::condition::ConditionExpressionRegistry* registry) {
    pathfinder::condition::ConditionExpressionEvaluator evaluator;
    auto evaluated = evaluator.evaluate(ref, buildDangerConditionContext(input, threat), registry);
    if (evaluated.is_error()) return Result<bool>::fail(evaluated.errors());
    return Result<bool>::ok(evaluated.value().matched);
}

pathfinder::condition::ConditionEvaluationContext buildDangerConditionContext(
    const HazardExposureInput& input,
    const ThreatProfile& threat) {
    pathfinder::condition::ConditionEvaluationContext context;
    context.context_type = "danger_hazard";
    context.tick = input.tick;
    context.safe_context_keys = input.safe_context_keys;

    pathfinder::condition::ActorConditionView actor;
    actor.requirement_keys = input.safe_context_keys;
    actor.capability_keys = input.safe_context_keys;
    actor.trust = input.tribe_projection.has_value() ? 0.5 : 0.0;
    context.actor = actor;

    pathfinder::condition::ObjectConditionView target;
    target.state_keys = threat.state_keys;
    target.tag_keys = threat.safe_tag_keys;
    context.target = target;
    context.source = target;

    pathfinder::condition::KnowledgeConditionView knowledge;
    for (const auto& claim : input.actor_knowledge_claims) {
        knowledge.canonical_claim_keys.push_back(claimKey(claim));
        knowledge.source_keys.push_back(claim.subject.subject_id);
        knowledge.condition_keys.push_back(claim.predicate.effect_key);
        for (const auto& condition : claim.conditions) {
            if (!condition.canonical_condition_key.empty()) knowledge.condition_keys.push_back(condition.canonical_condition_key);
        }
    }
    context.knowledge = knowledge;

    if (input.tribe_projection.has_value()) {
        pathfinder::condition::TribeConditionView tribe;
        tribe.cohesion = 0.5;
        tribe.trust = 0.5;
        tribe.capability_keys = input.tribe_projection->warning_keys;
        context.tribe = tribe;
    }

    return context;
}

Result<CountermeasureAssessment> CountermeasureEvaluator::assess(
    const HazardExposureInput& input,
    const ThreatProfile& threat,
    const HazardRule& rule,
    const pathfinder::condition::ConditionExpressionRegistry* registry) const {

    auto input_valid = input.validateBasic();
    if (input_valid.is_error()) return Result<CountermeasureAssessment>::fail(input_valid.errors());
    auto threat_valid = threat.validateBasic();
    if (threat_valid.is_error()) return Result<CountermeasureAssessment>::fail(threat_valid.errors());
    auto rule_valid = rule.validateBasic();
    if (rule_valid.is_error()) return Result<CountermeasureAssessment>::fail(rule_valid.errors());

    CountermeasureAssessment assessment;
    for (const auto& requirement : rule.countermeasure_requirements) {
        bool matched = false;
        if (!requirement.condition_ref.empty()) {
            auto condition = conditionMatches(requirement.condition_ref, input, threat, registry);
            if (condition.is_error()) return Result<CountermeasureAssessment>::fail(condition.errors());
            matched = condition.value();
        }
        if (!matched && containsValue(input.safe_context_keys, requirement.requirement_key)) matched = true;

        if (!matched && requirement.kind == CountermeasureKind::Tool) {
            matched = std::any_of(input.available_objects.begin(), input.available_objects.end(), [&](const auto& object) {
                return objectMatches(object, requirement.requirement_key);
            });
        }
        if (!matched && requirement.kind == CountermeasureKind::Knowledge) {
            matched = std::any_of(input.actor_knowledge_claims.begin(), input.actor_knowledge_claims.end(), [&](const auto& claim) {
                return claimMatches(claim, requirement.requirement_key, rule, threat);
            }) || std::any_of(input.tribe_shared_claims.begin(), input.tribe_shared_claims.end(), [&](const auto& claim) {
                return claimMatches(claim, requirement.requirement_key, rule, threat);
            });
        }
        if (!matched && requirement.kind == CountermeasureKind::GroupSupport && input.coordination_projection.has_value()) {
            matched = input.coordination_projection->pack_tactic.risk_reduction > 0.0;
        }
        if (!matched && requirement.kind == CountermeasureKind::EscapeRoute) {
            matched = containsValue(input.safe_context_keys, "escape_route");
        }

        if (matched) {
            assessment.mitigation_score = clampDangerRatio(assessment.mitigation_score + requirement.mitigation_score);
            appendUnique(assessment.applied_countermeasure_keys, requirement.requirement_key);
            for (const auto& reason : requirement.reason_keys) appendUnique(assessment.reason_keys, reason);
        }
    }

    auto valid = assessment.validateBasic();
    if (valid.is_error()) return Result<CountermeasureAssessment>::fail(valid.errors());
    return Result<CountermeasureAssessment>::ok(std::move(assessment));
}

static DangerDecision decisionFor(DangerSeverity severity, double mitigation, const CountermeasureAssessment& assessment, const HazardExposureInput& input) {
    if (containsValue(input.safe_context_keys, "escape_route") && mitigation >= 0.50) return DangerDecision::Escaped;
    if (mitigation >= 0.70 && severityRank(severity) <= severityRank(DangerSeverity::Notice)) return DangerDecision::Avoided;
    if (mitigation > 0.0 && severityRank(severity) <= severityRank(DangerSeverity::Strain)) return DangerDecision::Avoided;
    if (severity == DangerSeverity::None) return DangerDecision::Safe;
    if (severityRank(severity) <= severityRank(DangerSeverity::Notice)) return DangerDecision::Warned;
    return DangerDecision::Exposed;
}

static DangerFeedbackDraft makeFeedback(const HazardRule& rule, DangerSeverity severity, DangerDecision decision) {
    DangerFeedbackDraft feedback;
    feedback.feedback_key = rule.feedback_key.empty() ? "danger_feedback" : rule.feedback_key;
    feedback.safe_text = rule.feedback_text.empty() ? "你察觉到这里存在危险，需要谨慎行动。" : rule.feedback_text;
    feedback.visible_severity = severity;
    feedback.warning_keys = {toString(decision), toString(severity)};
    feedback.reason_keys = {rule.rule_key};
    return feedback;
}

static DangerKnowledgeDraft makeKnowledgeDraft(const ThreatProfile& threat, const HazardRule& rule, DangerSeverity severity) {
    DangerKnowledgeDraft draft;
    draft.knowledge_key = "danger_knowledge_" + rule.rule_key;
    draft.subject_id = !threat.source_definition_id.empty() ? threat.source_definition_id.value() : threat.threat_key;
    draft.action_key = toString(rule.trigger_kind);
    draft.effect_key = rule.knowledge_effect_key.empty() ? "danger_warning" : rule.knowledge_effect_key;
    draft.severity = severity;
    draft.reason_keys = {rule.rule_key, "danger_experience"};
    return draft;
}

static TribeDangerPressureDraft makeTribeDraft(const HazardExposureInput& input, const HazardRule& rule, DangerSeverity severity) {
    TribeDangerPressureDraft draft;
    draft.tribe_id = input.tribe_id;
    draft.morale_delta = rule.morale_delta;
    draft.trust_delta = rule.trust_delta;
    draft.safety_pressure = clampDangerRatio(rule.pressure_delta + severityRank(severity) * 0.10);
    draft.knowledge_conflict_pressure = rule.source_kind == DangerSourceKind::KnowledgeConflict ? clampDangerRatio(rule.pressure_delta) : 0.0;
    draft.casualty_pressure = severityRank(severity) >= severityRank(DangerSeverity::Harm) ? clampDangerRatio(rule.pressure_delta) : 0.0;
    draft.split_risk_hint = rule.split_risk_delta;
    draft.reason_keys = {rule.rule_key, "danger_pressure"};
    return draft;
}

Result<HazardResolutionResult> HazardResolver::resolve(
    const HazardExposureInput& input,
    const std::vector<HazardRule>& rules,
    const pathfinder::condition::ConditionExpressionRegistry* registry) const {

    auto input_valid = input.validateBasic();
    if (input_valid.is_error()) return Result<HazardResolutionResult>::fail(input_valid.errors());
    for (const auto& rule : rules) {
        auto rule_valid = rule.validateBasic();
        if (rule_valid.is_error()) return Result<HazardResolutionResult>::fail(rule_valid.errors());
    }

    HazardResolutionResult result;
    result.decision = DangerDecision::NoMatchingHazard;
    result.severity = DangerSeverity::None;
    result.trace.trace_key = "hazard_resolution";
    result.trace.decision = result.decision;
    result.trace.final_severity = result.severity;
    result.trace.reason_keys = {"p29_danger_resolver"};

    bool saw_blocked_rule = false;
    CountermeasureEvaluator countermeasures;
    for (const auto& threat : input.threats) {
        auto threat_valid = threat.validateBasic();
        if (threat_valid.is_error()) return Result<HazardResolutionResult>::fail(threat_valid.errors());
        for (const auto& rule : rules) {
            if (rule.trigger_kind != input.trigger_kind || rule.source_kind != threat.source_kind) continue;

            bool conditions_matched = true;
            for (const auto& ref : rule.condition_refs) {
                auto evaluated = conditionMatches(ref, input, threat, registry);
                if (evaluated.is_error()) return Result<HazardResolutionResult>::fail(evaluated.errors());
                appendUnique(result.trace.condition_trace_keys, ref.inline_canonical_key.empty() ? ref.expression_id.value() : ref.inline_canonical_key);
                if (!evaluated.value()) conditions_matched = false;
            }
            if (!conditions_matched) {
                saw_blocked_rule = true;
                appendUnique(result.trace.blocked_rule_keys, rule.rule_key);
                continue;
            }

            appendUnique(result.trace.matched_rule_keys, rule.rule_key);
            auto assessed = countermeasures.assess(input, threat, rule, registry);
            if (assessed.is_error()) return Result<HazardResolutionResult>::fail(assessed.errors());
            for (const auto& key : assessed.value().applied_countermeasure_keys) appendUnique(result.trace.applied_countermeasure_keys, key);

            const DangerSeverity base = severityRank(rule.severity) >= severityRank(threat.base_severity) ? rule.severity : threat.base_severity;
            const DangerSeverity final_severity = reduceSeverity(base, assessed.value().mitigation_score);
            const DangerDecision decision = decisionFor(final_severity, assessed.value().mitigation_score, assessed.value(), input);

            if (severityRank(final_severity) >= severityRank(result.severity)) {
                result.severity = final_severity;
                result.decision = decision;
            }

            result.feedbacks.push_back(makeFeedback(rule, final_severity, decision));
            if (severityRank(final_severity) >= severityRank(DangerSeverity::Notice)) {
                FearSignal signal;
                signal.kind = fearKindForSeverity(final_severity, !input.tribe_id.empty());
                signal.fear_pressure = clampDangerRatio(rule.fear_delta + threat.base_pressure);
                signal.confidence = clampDangerRatio(1.0 - threat.predictability + threat.visibility * 0.25);
                signal.affects_group = !input.tribe_id.empty();
                signal.reason_keys = {rule.rule_key, threat.threat_key};
                result.fear_signals.push_back(std::move(signal));
            }
            if (severityRank(final_severity) >= severityRank(DangerSeverity::Harm)) {
                InjuryStateDraft injury;
                injury.actor_id = input.actor_id;
                injury.severity = final_severity;
                injury.injury_key = "injury_pressure_" + toString(final_severity);
                injury.pressure_delta = clampDangerRatio(rule.pressure_delta);
                injury.blocks_action = severityRank(final_severity) >= severityRank(DangerSeverity::Severe);
                injury.requires_help = severityRank(final_severity) >= severityRank(DangerSeverity::Harm);
                injury.reason_keys = {rule.rule_key, threat.threat_key};
                result.injury_drafts.push_back(std::move(injury));
            }
            if (!rule.knowledge_effect_key.empty()) {
                result.knowledge_drafts.push_back(makeKnowledgeDraft(threat, rule, final_severity));
            }
            if (!input.tribe_id.empty()) {
                result.tribe_pressure_drafts.push_back(makeTribeDraft(input, rule, final_severity));
            }
            pathfinder::reaction::ReactionEventDraft event;
            event.event_key = "danger_event_" + rule.rule_key;
            event.reason_keys = {rule.rule_key, toString(decision)};
            result.event_drafts.push_back(std::move(event));
        }
    }

    if (result.trace.matched_rule_keys.empty()) {
        result.decision = saw_blocked_rule ? DangerDecision::BlockedByCondition : DangerDecision::NoMatchingHazard;
        result.severity = DangerSeverity::None;
        result.trace.decision = result.decision;
        result.trace.final_severity = result.severity;
        if (result.feedbacks.empty()) {
            DangerFeedbackDraft feedback;
            feedback.feedback_key = saw_blocked_rule ? "danger_blocked_by_condition" : "danger_no_matching_hazard";
            feedback.safe_text = saw_blocked_rule ? "危险条件没有成立，行动暂时没有触发危险。" : "没有发现明确危险。";
            feedback.visible_severity = DangerSeverity::None;
            feedback.warning_keys = {toString(result.decision)};
            feedback.reason_keys = {"p29_danger_resolver"};
            result.feedbacks.push_back(std::move(feedback));
        }
    } else {
        result.trace.decision = result.decision;
        result.trace.final_severity = result.severity;
    }

    auto valid = result.validateBasic();
    if (valid.is_error()) return Result<HazardResolutionResult>::fail(valid.errors());
    return Result<HazardResolutionResult>::ok(std::move(result));
}

} // namespace pathfinder::danger
