#include "pathfinder/knowledge/knowledge_formation.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;

// ============================================================
// Helpers
// ============================================================

static double clamp(double value, double min_val, double max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static bool isRiskRelation(KnowledgeRelationType type) {
    return type == KnowledgeRelationType::HasRisk ||
           type == KnowledgeRelationType::IsDangerousUnder;
}

// ============================================================
// KnowledgeFormationPlanner
// ============================================================

Result<KnowledgeFormationPlan> KnowledgeFormationPlanner::planFromMemorySummary(
    const KnowledgeFormationInput& input,
    const KnowledgeFormationOptions& options) const {

    auto input_result = input.validateBasic();
    if (input_result.is_error()) {
        return Result<KnowledgeFormationPlan>::fail(input_result.errors());
    }
    auto options_result = options.validateBasic();
    if (options_result.is_error()) {
        return Result<KnowledgeFormationPlan>::fail(options_result.errors());
    }

    const auto& summary = input.summary;

    // Check minimum evidence requirements
    if (summary.source_memory_count < options.min_source_memory_count) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "insufficient source memory count"));
    }
    if (summary.representative_memory_ids.size() < options.min_representative_count) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "insufficient representative memories"));
    }
    if (summary.evidence_refs.empty()) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "empty evidence_refs"));
    }
    if (summary.aggregate_strength < options.min_summary_strength) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "aggregate_strength below threshold"));
    }
    if (summary.key.summary_kind == pathfinder::memory::MemorySummaryKind::Unknown ||
        summary.key.summary_kind == pathfinder::memory::MemorySummaryKind::TestOnly) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::validation_failed, "invalid summary kind"));
    }
    if (!options.allow_contradiction_summary && summary.contradiction_count > 0) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "contradiction summary skipped"));
    }

    // Check safe predicate
    if (input.target_relation == KnowledgeRelationType::Unknown) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::validation_failed, "missing target_relation"));
    }
    if (input.target_relation == KnowledgeRelationType::TestOnly) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::validation_failed, "target_relation TestOnly not allowed"));
    }
    if (input.action_key.empty()) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::validation_failed, "missing action_key"));
    }

    // BLOCKER-6: HasEffect (and similar relations) must have effect_key
    static const std::vector<KnowledgeRelationType> effect_required_relations = {
        KnowledgeRelationType::HasEffect,
        KnowledgeRelationType::HasRisk,
        KnowledgeRelationType::IsEdibleUnder,
        KnowledgeRelationType::IsDangerousUnder,
        KnowledgeRelationType::IsUsableFor,
        KnowledgeRelationType::ReactsWith,
        KnowledgeRelationType::TransformsInto,
        KnowledgeRelationType::Produces,
        KnowledgeRelationType::Prevents,
        KnowledgeRelationType::FailsUnder,
    };
    bool requires_effect = false;
    for (const auto& rt : effect_required_relations) {
        if (input.target_relation == rt) {
            requires_effect = true;
            break;
        }
    }
    if (requires_effect && input.effect_key.empty()) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::validation_failed, "target_relation requires effect_key"));
    }

    // Build subject from summary
    KnowledgeSubject subject;
    subject.kind = KnowledgeSubjectKind::ObjectDefinition;
    subject.subject_id = summary.key.subject.subject_id;
    subject.subject_type_key = summary.key.subject.subject_type_key;
    subject.related_subject_ids = input.related_subject_ids;
    subject.relation_group_key = input.relation_group_key;
    subject.safe_tags = summary.key.subject.safe_tags;

    // Build predicate
    KnowledgePredicate predicate;
    predicate.relation_type = input.target_relation;
    predicate.action_key = input.action_key;
    predicate.effect_key = input.effect_key;

    // Build evidence refs
    std::vector<KnowledgeEvidence> evidence_refs;
    KnowledgeIdFactory id_factory;
    size_t evidence_index = 0;

    // First, create a placeholder knowledge_id for evidence generation
    // We'll use a temporary one since the real ID needs the plan
    std::string temp_know_id = "temp_plan_evidence";

    // Evidence from summary itself
    {
        KnowledgeEvidence ev;
        auto eid_result = id_factory.makeEvidenceId(
            pathfinder::foundation::KnowledgeId(temp_know_id), evidence_index++);
        if (eid_result.is_ok()) {
            ev.evidence_id = eid_result.value();
        }
        ev.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
        ev.source_summary_id = summary.summary_id;
        ev.source_memory_ids = summary.source_memory_ids;
        ev.memory_evidence_refs = summary.evidence_refs;
        ev.supports_claim = true;
        ev.confidence_delta = summary.aggregate_strength * 0.5;
        ev.reliability = 0.9;
        ev.observed_tick = summary.last_observed_tick;
        evidence_refs.push_back(ev);
    }

    // Evidence from representative records
    for (const auto& record : input.representative_records) {
        if (evidence_refs.size() >= options.max_evidence_refs) break;
        KnowledgeEvidence ev;
        auto eid_result = id_factory.makeEvidenceId(
            pathfinder::foundation::KnowledgeId(temp_know_id), evidence_index++);
        if (eid_result.is_ok()) {
            ev.evidence_id = eid_result.value();
        }
        ev.evidence_kind = KnowledgeEvidenceKind::MemoryRecord;
        ev.source_memory_ids.push_back(record.memory_id);
        ev.memory_evidence_refs = record.evidence_refs;
        ev.supports_claim = true;
        ev.confidence_delta = record.strength * 0.1;
        ev.reliability = 0.8;
        ev.observed_tick = record.last_touched_tick;
        evidence_refs.push_back(ev);
    }

    // Confidence calculation
    double base = summary.aggregate_strength * 0.55;
    double sample_bonus = std::min(static_cast<double>(summary.source_memory_count) / 10.0, 1.0) * 0.20;
    double long_term_bonus = summary.has_long_term_source ? 0.10 : 0.0;
    double teaching_bonus = std::min(static_cast<double>(summary.teaching_candidate_count) / 5.0, 1.0) * 0.05;

    double risk_penalty = 0.0;
    double contradiction_penalty = 0.0;
    if (!isRiskRelation(input.target_relation)) {
        if ((summary.warning_count + summary.accident_count) > 0) {
            risk_penalty = 0.05;
        }
    }
    if (summary.contradiction_count > 0) {
        contradiction_penalty = 0.20;
    }

    double confidence = clamp(base + sample_bonus + long_term_bonus + teaching_bonus - risk_penalty - contradiction_penalty, 0.0, 1.0);

    // Status decision
    KnowledgeStatus projected_status;
    if (confidence < options.min_hypothesis_confidence) {
        return Result<KnowledgeFormationPlan>::fail(
            makeError(ErrorCode::knowledge_insufficient_confidence, "confidence below Hypothesis threshold"));
    } else if (confidence < options.active_confidence_threshold) {
        projected_status = KnowledgeStatus::Hypothesis;
    } else if (confidence < options.teachable_confidence_threshold) {
        projected_status = KnowledgeStatus::Active;
    } else {
        projected_status = KnowledgeStatus::Teachable;
    }

    // Build conditions from input through the P27 condition normalizer.
    auto conditions_result = normalizeKnowledgeConditions(input.candidate_conditions);
    if (conditions_result.is_error()) {
        return Result<KnowledgeFormationPlan>::fail(conditions_result.errors());
    }
    std::vector<KnowledgeCondition> conditions = conditions_result.value();

    // Build confidence DTO
    KnowledgeConfidence projected_confidence;
    projected_confidence.confidence = confidence;
    projected_confidence.band = confidenceToBand(confidence);
    projected_confidence.support_count = evidence_refs.size();
    projected_confidence.conflict_count = summary.contradiction_count;
    projected_confidence.source_memory_count = summary.source_memory_ids.size();
    projected_confidence.source_summary_count = 1;
    projected_confidence.long_term_source_count = summary.has_long_term_source ? 1 : 0;

    KnowledgeFormationPlan plan;
    plan.plan_key = "plan_from_summary_" + summary.summary_id.value();
    plan.input = input;
    plan.input.candidate_conditions = conditions;
    plan.subject = subject;
    plan.predicate = predicate;
    plan.evidence_refs = evidence_refs;
    plan.projected_confidence = projected_confidence;
    plan.projected_status = projected_status;
    plan.reason_keys.push_back("planned_from_memory_summary");

    return Result<KnowledgeFormationPlan>::ok(std::move(plan));
}

// ============================================================
// KnowledgeFormationService
// ============================================================

Result<KnowledgeFormationResult> KnowledgeFormationService::formFromMemorySummary(
    const KnowledgeFormationInput& input,
    const KnowledgeFormationOptions& options) const {

    KnowledgeFormationPlanner planner;
    auto plan_result = planner.planFromMemorySummary(input, options);

    KnowledgeFormationResult result;
    result.decision = KnowledgeFormationDecision::Unknown;

    if (plan_result.is_error()) {
        // BLOCKER-2: Only convert insufficient_confidence to Skipped.
        // Validation/security errors must fail.
        bool is_insufficient = false;
        for (const auto& err : plan_result.errors()) {
            if (err.code == ErrorCode::knowledge_insufficient_confidence) {
                is_insufficient = true;
            }
        }
        if (!is_insufficient) {
            return Result<KnowledgeFormationResult>::fail(plan_result.errors());
        }

        // BLOCKER-1: Skipped result must not contain invalid event drafts.
        result.decision = KnowledgeFormationDecision::Skipped;
        result.reason_keys.push_back("insufficient_evidence");
        for (const auto& err : plan_result.errors()) {
            if (err.debug_message.has_value()) {
                result.reason_keys.push_back(err.debug_message.value());
            }
        }
        // No event_drafts or state_changes for Skipped.
        return Result<KnowledgeFormationResult>::ok(std::move(result));
    }

    const auto& plan = plan_result.value();

    // Generate knowledge ID
    KnowledgeIdFactory id_factory;
    auto know_id_result = id_factory.makeKnowledgeId(
        input.owner,
        plan.subject,
        plan.predicate,
        input.summary.summarized_tick,
        0);

    if (know_id_result.is_error()) {
        return Result<KnowledgeFormationResult>::fail(know_id_result.errors());
    }

    auto knowledge_id = know_id_result.value();

    // Rebuild evidence with real knowledge ID
    std::vector<KnowledgeEvidence> final_evidence = plan.evidence_refs;
    for (size_t i = 0; i < final_evidence.size(); ++i) {
        auto ev_id_result = id_factory.makeEvidenceId(knowledge_id, i);
        if (ev_id_result.is_ok()) {
            final_evidence[i].evidence_id = ev_id_result.value();
        }
    }

    // Build teaching profile
    KnowledgeTeachingProfile teaching_profile;
    if (plan.projected_status == KnowledgeStatus::Teachable ||
        plan.projected_status == KnowledgeStatus::Active) {
        teaching_profile.teachable = true;
        teaching_profile.teaching_message_key = "knowledge.teachable." + plan.subject.subject_id + "." + toString(plan.predicate.relation_type);
        if (isRiskRelation(plan.predicate.relation_type)) {
            teaching_profile.risk_warning_required = true;
        }
    }

    // Build projection flags
    KnowledgeProjectionFlags projection_flags;
    projection_flags.usable_by_ai = true;
    projection_flags.usable_for_action = (plan.projected_status == KnowledgeStatus::Active ||
                                           plan.projected_status == KnowledgeStatus::Teachable);
    projection_flags.usable_for_teaching = teaching_profile.teachable;

    // Build claim
    KnowledgeClaim claim;
    claim.knowledge_id = knowledge_id;
    claim.owner = input.owner;
    claim.subject = plan.subject;
    claim.predicate = plan.predicate;
    claim.conditions = plan.input.candidate_conditions;
    claim.confidence = plan.projected_confidence;
    claim.status = plan.projected_status;
    claim.evidence_refs = final_evidence;
    claim.teaching_profile = teaching_profile;
    claim.projection_flags = projection_flags;
    claim.created_tick = input.summary.summarized_tick;
    claim.updated_tick = input.summary.summarized_tick;
    claim.reason_keys.push_back("formed_from_memory_summary");

    auto trace_result = id_factory.makeTraceId(knowledge_id, 0);
    if (trace_result.is_ok()) {
        claim.trace_ids.push_back(trace_result.value());
    }

    // Build result
    result.decision = KnowledgeFormationDecision::CreatedClaim;
    result.plan = plan;
    result.claim = claim;
    result.reason_keys.push_back("created_claim_from_summary");

    result.event_drafts.push_back(KnowledgeEventDraft{
        .event_key = "knowledge.claim_created",
        .knowledge_id = knowledge_id,
        .owner = input.owner,
        .subject = plan.subject,
        .relation_type = plan.predicate.relation_type,
        .status = plan.projected_status,
        .decision = KnowledgeFormationDecision::CreatedClaim,
        .reason_keys = {"knowledge.claim_created"}
    });

    result.state_changes.push_back(KnowledgeStateChangeDraft{
        .change_key = "knowledge.state_change.created",
        .knowledge_id = knowledge_id,
        .reason_keys = {"knowledge_created"}
    });

    return Result<KnowledgeFormationResult>::ok(std::move(result));
}

} // namespace pathfinder::knowledge
