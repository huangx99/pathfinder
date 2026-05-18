#include "pathfinder/knowledge/knowledge_revision.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::KnowledgeId;

// ============================================================
// Helpers
// ============================================================

static double clamp(double value, double min_val, double max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static KnowledgeStatus confidenceToStatus(double confidence) {
    if (confidence < 0.10) return KnowledgeStatus::Candidate;
    if (confidence < 0.35) return KnowledgeStatus::Hypothesis;
    if (confidence < 0.75) return KnowledgeStatus::Active;
    return KnowledgeStatus::Teachable;
}

static bool isMatchingClaim(const KnowledgeClaim& existing, const KnowledgeFormationInput& incoming) {
    // P19 first version: conservative matching only
    // Owner must match
    if (existing.owner != incoming.owner) return false;
    // Subject must be identical
    if (existing.subject.subject_id != incoming.summary.key.subject.subject_id) return false;
    // Action key must match
    if (existing.predicate.action_key != incoming.action_key) return false;
    // Relation type must match
    if (existing.predicate.relation_type != incoming.target_relation) return false;
    return true;
}

static KnowledgePredicate makeIncomingPredicate(const KnowledgeFormationInput& input) {
    KnowledgePredicate pred;
    pred.relation_type = input.target_relation;
    pred.action_key = input.action_key;
    pred.effect_key = input.effect_key;
    return pred;
}

static bool isCompatibleEffect(const std::string& existing_effect, const std::string& incoming_effect) {
    if (existing_effect.empty() || incoming_effect.empty()) return true;
    return existing_effect == incoming_effect;
}

static bool hasSameConditions(const std::vector<KnowledgeCondition>& a, const std::vector<KnowledgeCondition>& b) {
    if (a.size() != b.size()) return false;
    std::vector<std::string> left;
    std::vector<std::string> right;
    left.reserve(a.size());
    right.reserve(b.size());
    for (const auto& condition : a) {
        auto canonical = canonicalKnowledgeConditionKey(condition);
        if (canonical.is_error()) return false;
        left.push_back(canonical.value());
    }
    for (const auto& condition : b) {
        auto canonical = canonicalKnowledgeConditionKey(condition);
        if (canonical.is_error()) return false;
        right.push_back(canonical.value());
    }
    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());
    return left == right;
}

static bool hasNegativeEvidence(const std::vector<KnowledgeEvidence>& evidence_refs) {
    for (const auto& ev : evidence_refs) {
        if (!ev.supports_claim) return true;
    }
    return false;
}

// ============================================================
// KnowledgeRevisionOptions
// ============================================================

Result<void> KnowledgeRevisionOptions::validateBasic() const {
    if (reinforce_weight < 0.0 || reinforce_weight > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeRevisionOptions reinforce_weight out of range"));
    }
    if (weak_conflict_penalty < 0.0 || weak_conflict_penalty > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeRevisionOptions weak_conflict_penalty out of range"));
    }
    if (strong_conflict_penalty < 0.0 || strong_conflict_penalty > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeRevisionOptions strong_conflict_penalty out of range"));
    }
    if (disprove_confidence_max < 0.0 || disprove_confidence_max > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeRevisionOptions disprove_confidence_max out of range"));
    }
    if (strong_conflict_min_count == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionOptions strong_conflict_min_count is 0"));
    }
    if (max_revision_evidence_refs == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionOptions max_revision_evidence_refs is 0"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeConflictSignal
// ============================================================

Result<void> KnowledgeConflictSignal::validateBasic() const {
    if (conflict_type == KnowledgeConflictType::Unknown ||
        conflict_type == KnowledgeConflictType::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConflictSignal conflict_type is invalid"));
    }
    if (existing_knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConflictSignal existing_knowledge_id is empty"));
    }
    auto subject_result = subject.validateBasic();
    if (subject_result.is_error()) return subject_result;
    auto existing_pred_result = existing_predicate.validateBasic();
    if (existing_pred_result.is_error()) return existing_pred_result;
    auto incoming_pred_result = incoming_predicate.validateBasic();
    if (incoming_pred_result.is_error()) return incoming_pred_result;
    for (const auto& cond : existing_conditions) {
        auto r = cond.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& cond : incoming_conditions) {
        auto r = cond.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& ev : evidence_refs) {
        auto r = ev.validateBasic();
        if (r.is_error()) return r;
    }
    if (severity < 0.0 || severity > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeConflictSignal severity out of range"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConflictSignal reason_keys contain forbidden key"));
    }
    if (should_specialize) {
        if (incoming_conditions.empty() && existing_conditions.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeConflictSignal should_specialize requires non-empty conditions"));
        }
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRevisionInput
// ============================================================

Result<void> KnowledgeRevisionInput::validateBasic() const {
    auto formation_result = formation_input.validateBasic();
    if (formation_result.is_error()) return formation_result;
    for (const auto& claim : existing_claims) {
        auto claim_result = claim.validateBasic();
        if (claim_result.is_error()) return claim_result;
        if (claim.owner != formation_input.owner) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionInput existing_claim owner mismatch"));
        }
        if (claim.status == KnowledgeStatus::TestOnly) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionInput existing_claim TestOnly not allowed"));
        }
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionInput reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRevisionAssessment
// ============================================================

Result<void> KnowledgeRevisionAssessment::validateBasic() const {
    if (recommended_strategy == KnowledgeResolutionStrategy::Unknown ||
        recommended_strategy == KnowledgeResolutionStrategy::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionAssessment recommended_strategy is invalid"));
    }
    if (confidence_delta < -1.0 || confidence_delta > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "KnowledgeRevisionAssessment confidence_delta out of range"));
    }
    if (has_conflict && conflict_signals.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionAssessment has_conflict requires conflict_signals"));
    }
    if (should_disprove_existing && !has_conflict) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionAssessment should_disprove_existing requires has_conflict"));
    }
    if (should_create_specialized_claim) {
        if (recommended_strategy != KnowledgeResolutionStrategy::Specialize &&
            recommended_strategy != KnowledgeResolutionStrategy::KeepBoth) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionAssessment should_create_specialized_claim requires Specialize or KeepBoth strategy"));
        }
    }
    for (const auto& signal : conflict_signals) {
        auto r = signal.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionAssessment reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeClaimPatch
// ============================================================

Result<void> KnowledgeClaimPatch::validateBasic() const {
    if (knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaimPatch knowledge_id is empty"));
    }
    if (status_before == KnowledgeStatus::Unknown || status_before == KnowledgeStatus::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaimPatch status_before is invalid"));
    }
    if (status_after == KnowledgeStatus::Unknown || status_after == KnowledgeStatus::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaimPatch status_after is invalid"));
    }
    auto conf_before_result = confidence_before.validateBasic();
    if (conf_before_result.is_error()) return conf_before_result;
    auto conf_after_result = confidence_after.validateBasic();
    if (conf_after_result.is_error()) return conf_after_result;
    for (const auto& ev : added_evidence_refs) {
        auto r = ev.validateBasic();
        if (r.is_error()) return r;
    }
    if (status_after == KnowledgeStatus::Disproven) {
        if (!hasNegativeEvidence(added_evidence_refs)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaimPatch Disproven requires negative evidence"));
        }
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeClaimPatch reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRevisionPlan
// ============================================================

Result<void> KnowledgeRevisionPlan::validateBasic() const {
    if (plan_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionPlan plan_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(plan_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionPlan plan_key contains forbidden key"));
    }
    auto input_result = input.validateBasic();
    if (input_result.is_error()) return input_result;
    auto assessment_result = assessment.validateBasic();
    if (assessment_result.is_error()) return assessment_result;
    for (const auto& patch : patches) {
        auto r = patch.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& claim : candidate_new_claims) {
        auto r = claim.validateBasic();
        if (r.is_error()) return r;
        // New specialized claims must have evidence and supersedes relation
        if (claim.evidence_refs.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionPlan candidate_new_claims requires evidence_refs"));
        }
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionPlan reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionPlan warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRevisionResult
// ============================================================

bool KnowledgeRevisionResult::ok() const {
    return decision == KnowledgeRevisionDecision::Skipped ||
           decision == KnowledgeRevisionDecision::NoChange ||
           decision == KnowledgeRevisionDecision::ReinforcedClaim ||
           decision == KnowledgeRevisionDecision::WeakenedClaim ||
           decision == KnowledgeRevisionDecision::CreatedSpecializedClaim ||
           decision == KnowledgeRevisionDecision::DeprecatedClaim ||
           decision == KnowledgeRevisionDecision::DisprovenClaim;
}

Result<void> KnowledgeRevisionResult::validateBasic() const {
    if (decision == KnowledgeRevisionDecision::Unknown ||
        decision == KnowledgeRevisionDecision::TestOnly ||
        decision == KnowledgeRevisionDecision::Rejected) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult decision is invalid"));
    }
    if (plan.has_value()) {
        auto r = plan->validateBasic();
        if (r.is_error()) return r;
    }
    if (decision == KnowledgeRevisionDecision::ReinforcedClaim ||
        decision == KnowledgeRevisionDecision::WeakenedClaim ||
        decision == KnowledgeRevisionDecision::DeprecatedClaim ||
        decision == KnowledgeRevisionDecision::DisprovenClaim) {
        if (updated_claims.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult requires updated_claims"));
        }
        if (applied_patches.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult requires applied_patches"));
        }
    }
    if (decision == KnowledgeRevisionDecision::CreatedSpecializedClaim) {
        if (created_claims.empty()) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult CreatedSpecializedClaim requires created_claims"));
        }
    }
    for (const auto& claim : updated_claims) {
        auto r = claim.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& claim : created_claims) {
        auto r = claim.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& patch : applied_patches) {
        auto r = patch.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& draft : event_drafts) {
        auto r = draft.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& change : state_changes) {
        auto r = change.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "KnowledgeRevisionResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRevisionPlanner
// ============================================================

Result<KnowledgeRevisionPlan> KnowledgeRevisionPlanner::planRevision(
    const KnowledgeRevisionInput& input,
    const KnowledgeRevisionOptions& options) const {

    auto input_result = input.validateBasic();
    if (input_result.is_error()) {
        return Result<KnowledgeRevisionPlan>::fail(input_result.errors());
    }
    auto options_result = options.validateBasic();
    if (options_result.is_error()) {
        return Result<KnowledgeRevisionPlan>::fail(options_result.errors());
    }

    KnowledgeRevisionPlan plan;
    plan.plan_key = "revision_plan_" + std::to_string(input.revision_tick.value());
    plan.input = input;

    const auto& summary = input.formation_input.summary;
    const auto& incoming_owner = input.formation_input.owner;

    // Match existing claims
    std::vector<KnowledgeClaim> matched_claims;
    for (const auto& claim : input.existing_claims) {
        if (isMatchingClaim(claim, input.formation_input)) {
            matched_claims.push_back(claim);
            plan.assessment.matched_claim_ids.push_back(claim.knowledge_id);
        }
    }

    // Build incoming predicate from formation input
    KnowledgePredicate incoming_predicate = makeIncomingPredicate(input.formation_input);

    // Build incoming evidence from summary
    KnowledgeEvidence incoming_evidence;
    KnowledgeIdFactory id_factory;
    auto temp_know_id = pathfinder::foundation::KnowledgeId("temp_revision_evidence");
    auto eid_result = id_factory.makeEvidenceId(temp_know_id, 0);
    if (eid_result.is_ok()) {
        incoming_evidence.evidence_id = eid_result.value();
    }
    incoming_evidence.evidence_kind = KnowledgeEvidenceKind::MemorySummary;
    incoming_evidence.source_summary_id = summary.summary_id;
    incoming_evidence.source_memory_ids = summary.source_memory_ids;
    incoming_evidence.memory_evidence_refs = summary.evidence_refs;
    incoming_evidence.confidence_delta = summary.aggregate_strength;
    incoming_evidence.reliability = 0.9;
    incoming_evidence.observed_tick = summary.last_observed_tick;

    // If no matched claims, return Skipped plan (empty patches, no candidates)
    if (matched_claims.empty()) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::KeepBoth;
        plan.assessment.has_support = false;
        plan.assessment.has_conflict = false;
        plan.reason_keys.push_back("no_matching_claims");
        plan.warning_keys.push_back("no_match_assessment_placeholder");
        return Result<KnowledgeRevisionPlan>::ok(std::move(plan));
    }

    // Analyze each matched claim independently
    bool overall_has_support = false;
    bool overall_has_conflict = false;
    bool should_specialize = false;
    bool should_deprecate = false;
    bool should_disprove = false;

    for (const auto& claim : matched_claims) {
        KnowledgeConflictSignal signal;
        signal.existing_knowledge_id = claim.knowledge_id;
        signal.subject = claim.subject;
        signal.existing_predicate = claim.predicate;
        signal.incoming_predicate = incoming_predicate;
        signal.existing_conditions = claim.conditions;
        signal.incoming_conditions = input.formation_input.candidate_conditions;
        signal.evidence_refs.push_back(incoming_evidence);
        signal.severity = summary.aggregate_strength;

        // Step 1: Determine if this is support or conflict
        // Must compare effect_key compatibility first
        bool same_conditions = hasSameConditions(claim.conditions, input.formation_input.candidate_conditions);
        bool compatible_effect = isCompatibleEffect(claim.predicate.effect_key, incoming_predicate.effect_key);

        bool claim_has_conflict = false;
        bool claim_has_support = false;

        if (!compatible_effect) {
            // Effect mismatch: always a conflict, regardless of contradiction_count
            claim_has_conflict = true;
            if (!same_conditions &&
                (!claim.conditions.empty() || !input.formation_input.candidate_conditions.empty()) &&
                options.prefer_specialization) {
                signal.conflict_type = KnowledgeConflictType::ConditionMismatch;
                signal.condition_explains_conflict = true;
                signal.should_specialize = true;
                should_specialize = true;
            } else if (same_conditions && summary.contradiction_count >= static_cast<int>(options.strong_conflict_min_count)) {
                signal.conflict_type = KnowledgeConflictType::StrongContradiction;
                signal.severity = 1.0;
                double projected_confidence = claim.confidence.confidence -
                    summary.aggregate_strength * options.strong_conflict_penalty;
                if (projected_confidence <= options.disprove_confidence_max && options.allow_disproven) {
                    should_disprove = true;
                }
            } else {
                signal.conflict_type = KnowledgeConflictType::OppositeEffect;
            }
        } else {
            // Effect compatible: use contradiction_count to distinguish support vs conflict
            if (summary.contradiction_count == 0) {
                claim_has_support = true;
                signal.conflict_type = KnowledgeConflictType::SameClaimSupport;
                signal.should_specialize = false;
            } else {
                claim_has_conflict = true;
                if (!same_conditions &&
                    (!claim.conditions.empty() || !input.formation_input.candidate_conditions.empty()) &&
                    options.prefer_specialization) {
                    signal.conflict_type = KnowledgeConflictType::ConditionMismatch;
                    signal.condition_explains_conflict = true;
                    signal.should_specialize = true;
                    should_specialize = true;
                } else if (same_conditions && summary.contradiction_count >= static_cast<int>(options.strong_conflict_min_count)) {
                    signal.conflict_type = KnowledgeConflictType::StrongContradiction;
                    signal.severity = 1.0;
                    double projected_confidence = claim.confidence.confidence -
                        summary.aggregate_strength * options.strong_conflict_penalty;
                    if (projected_confidence <= options.disprove_confidence_max && options.allow_disproven) {
                        should_disprove = true;
                    }
                } else {
                    signal.conflict_type = KnowledgeConflictType::WeakContradiction;
                }
            }
        }

        if (claim_has_support) overall_has_support = true;
        if (claim_has_conflict) overall_has_conflict = true;

        signal.reason_keys.push_back("detected_conflict_or_support");
        plan.assessment.conflict_signals.push_back(signal);
    }

    plan.assessment.has_support = overall_has_support;
    plan.assessment.has_conflict = overall_has_conflict;
    plan.assessment.should_create_specialized_claim = should_specialize;
    plan.assessment.should_deprecate_existing = should_deprecate;
    plan.assessment.should_disprove_existing = should_disprove;

    // Determine recommended strategy
    if (should_specialize) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::Specialize;
        plan.assessment.confidence_delta = 0.0;
    } else if (should_disprove) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::Disprove;
        plan.assessment.confidence_delta = -summary.aggregate_strength * options.strong_conflict_penalty;
    } else if (should_deprecate) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::Deprecate;
        plan.assessment.confidence_delta = -summary.aggregate_strength * options.weak_conflict_penalty;
    } else if (overall_has_conflict) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::Weaken;
        plan.assessment.confidence_delta = -summary.aggregate_strength * options.weak_conflict_penalty;
    } else if (overall_has_support) {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::Reinforce;
        plan.assessment.confidence_delta = summary.aggregate_strength * options.reinforce_weight;
    } else {
        plan.assessment.recommended_strategy = KnowledgeResolutionStrategy::KeepBoth;
        plan.assessment.confidence_delta = 0.0;
    }

    // Generate patches for each matched claim (per-claim logic)
    for (size_t claim_idx = 0; claim_idx < matched_claims.size(); ++claim_idx) {
        const auto& claim = matched_claims[claim_idx];
        const auto& signal = plan.assessment.conflict_signals[claim_idx];
        bool claim_conflict = (signal.conflict_type != KnowledgeConflictType::SameClaimSupport);

        KnowledgeClaimPatch patch;
        patch.knowledge_id = claim.knowledge_id;
        patch.status_before = claim.status;
        patch.confidence_before = claim.confidence;

        double new_confidence = clamp(claim.confidence.confidence + plan.assessment.confidence_delta, 0.0, 1.0);
        patch.confidence_after.confidence = new_confidence;
        patch.confidence_after.band = confidenceToBand(new_confidence);
        patch.confidence_after.support_count = claim.confidence.support_count;
        patch.confidence_after.conflict_count = claim.confidence.conflict_count + (claim_conflict ? 1 : 0);
        patch.confidence_after.source_memory_count = claim.confidence.source_memory_count;
        patch.confidence_after.source_summary_count = claim.confidence.source_summary_count + 1;
        patch.confidence_after.long_term_source_count = claim.confidence.long_term_source_count;
        patch.confidence_after.last_delta = plan.assessment.confidence_delta;
        patch.confidence_after.last_verified_tick = claim.confidence.last_verified_tick;
        patch.confidence_after.last_contradicted_tick = claim_conflict ? input.revision_tick : claim.confidence.last_contradicted_tick;

        // Add incoming evidence with per-claim supports_claim
        KnowledgeEvidence ev = incoming_evidence;
        auto ev_id_result = id_factory.makeEvidenceId(claim.knowledge_id, claim.evidence_refs.size());
        if (ev_id_result.is_ok()) {
            ev.evidence_id = ev_id_result.value();
        }
        ev.supports_claim = !claim_conflict;
        patch.added_evidence_refs.push_back(ev);

        // Determine new status
        if (plan.assessment.recommended_strategy == KnowledgeResolutionStrategy::Disprove &&
            options.allow_disproven) {
            patch.status_after = KnowledgeStatus::Disproven;
            patch.reason_keys.push_back("disproven_by_revision");
        } else if (plan.assessment.recommended_strategy == KnowledgeResolutionStrategy::Deprecate &&
                   options.allow_deprecated) {
            patch.status_after = KnowledgeStatus::Deprecated;
            patch.reason_keys.push_back("deprecated_by_revision");
        } else if (plan.assessment.recommended_strategy == KnowledgeResolutionStrategy::Weaken) {
            patch.status_after = confidenceToStatus(new_confidence);
            patch.reason_keys.push_back("weakened_by_conflict");
        } else if (plan.assessment.recommended_strategy == KnowledgeResolutionStrategy::Reinforce) {
            patch.status_after = confidenceToStatus(new_confidence);
            patch.reason_keys.push_back("reinforced_by_support");
        } else if (plan.assessment.recommended_strategy == KnowledgeResolutionStrategy::Specialize) {
            patch.status_after = options.allow_deprecated ? KnowledgeStatus::Deprecated : claim.status;
            patch.reason_keys.push_back("deprecated_by_specialization");
        } else {
            patch.status_after = confidenceToStatus(new_confidence);
        }

        plan.patches.push_back(patch);
    }

    // Generate candidate new claims for specialization using incoming predicate
    std::vector<pathfinder::foundation::KnowledgeId> new_knowledge_ids;
    if (should_specialize && options.allow_create_specialized_claim) {
        for (const auto& claim : matched_claims) {
            KnowledgeClaim new_claim;
            auto new_know_id_result = id_factory.makeKnowledgeId(
                incoming_owner,
                claim.subject,
                incoming_predicate,
                input.revision_tick,
                1);
            if (new_know_id_result.is_ok()) {
                new_claim.knowledge_id = new_know_id_result.value();
            } else {
                new_claim.knowledge_id = pathfinder::foundation::KnowledgeId(
                    "know_specialized_" + claim.knowledge_id.value() + "_" + std::to_string(input.revision_tick.value()));
            }
            new_claim.owner = incoming_owner;
            new_claim.subject = claim.subject;
            new_claim.predicate = incoming_predicate;
            new_claim.conditions = input.formation_input.candidate_conditions;
            new_claim.confidence.confidence = summary.aggregate_strength;
            new_claim.confidence.band = confidenceToBand(summary.aggregate_strength);
            new_claim.confidence.support_count = 1;
            new_claim.confidence.conflict_count = 0;
            new_claim.status = confidenceToStatus(summary.aggregate_strength);

            // Evidence
            KnowledgeEvidence ev = incoming_evidence;
            auto ev_id_result = id_factory.makeEvidenceId(new_claim.knowledge_id, 0);
            if (ev_id_result.is_ok()) {
                ev.evidence_id = ev_id_result.value();
            }
            ev.supports_claim = true;
            new_claim.evidence_refs.push_back(ev);

            new_claim.supersedes_knowledge_ids.push_back(claim.knowledge_id);
            new_claim.created_tick = input.revision_tick;
            new_claim.updated_tick = input.revision_tick;
            new_claim.reason_keys.push_back("specialized_from_existing");

            // Update teaching_profile if Teachable
            if (new_claim.status == KnowledgeStatus::Teachable) {
                new_claim.teaching_profile.teachable = true;
                new_claim.teaching_profile.teaching_message_key = "knowledge.teachable." + new_claim.subject.subject_id + "." + toString(new_claim.predicate.relation_type);
                new_claim.projection_flags.usable_for_teaching = true;
            }
            if (new_claim.status == KnowledgeStatus::Active || new_claim.status == KnowledgeStatus::Teachable) {
                new_claim.projection_flags.usable_for_action = true;
            }

            plan.candidate_new_claims.push_back(new_claim);
            new_knowledge_ids.push_back(new_claim.knowledge_id);
        }
    }

    // Link patches to new claims for specialization
    if (should_specialize && !plan.candidate_new_claims.empty()) {
        for (size_t i = 0; i < plan.patches.size() && i < new_knowledge_ids.size(); ++i) {
            plan.patches[i].superseded_by_knowledge_id = new_knowledge_ids[i];
        }
    }

    plan.reason_keys.push_back("revision_planned");
    return Result<KnowledgeRevisionPlan>::ok(std::move(plan));
}

// ============================================================
// KnowledgeRevisionService
// ============================================================

Result<KnowledgeRevisionResult> KnowledgeRevisionService::revise(
    const KnowledgeRevisionInput& input,
    const KnowledgeRevisionOptions& options) const {

    KnowledgeRevisionPlanner planner;
    auto plan_result = planner.planRevision(input, options);

    KnowledgeRevisionResult result;
    result.decision = KnowledgeRevisionDecision::Unknown;

    if (plan_result.is_error()) {
        bool is_validation = false;
        for (const auto& err : plan_result.errors()) {
            if (err.code == ErrorCode::validation_failed ||
                err.code == ErrorCode::validation_value_out_of_range ||
                err.code == ErrorCode::validation_enum_unknown) {
                is_validation = true;
            }
        }
        if (is_validation) {
            return Result<KnowledgeRevisionResult>::fail(plan_result.errors());
        }
        // Non-validation errors -> Skipped
        result.decision = KnowledgeRevisionDecision::Skipped;
        result.reason_keys.push_back("plan_failed");
        for (const auto& err : plan_result.errors()) {
            if (err.debug_message.has_value()) {
                result.reason_keys.push_back(err.debug_message.value());
            }
        }
        return Result<KnowledgeRevisionResult>::ok(std::move(result));
    }

    const auto& plan = plan_result.value();

    // If no matching claims and no candidate new claims -> Skipped/NoChange
    if (plan.assessment.matched_claim_ids.empty() && plan.candidate_new_claims.empty()) {
        result.decision = KnowledgeRevisionDecision::Skipped;
        result.plan = plan;
        result.reason_keys.push_back("no_matching_claims_skipped");
        return Result<KnowledgeRevisionResult>::ok(std::move(result));
    }

    // Build updated claims from patches
    for (const auto& patch : plan.patches) {
        // Find original claim
        const KnowledgeClaim* original = nullptr;
        for (const auto& claim : input.existing_claims) {
            if (claim.knowledge_id == patch.knowledge_id) {
                original = &claim;
                break;
            }
        }
        if (!original) continue;

        KnowledgeClaim updated = *original;
        updated.status = patch.status_after;
        updated.confidence = patch.confidence_after;
        for (const auto& ev : patch.added_evidence_refs) {
            if (updated.evidence_refs.size() < options.max_revision_evidence_refs) {
                updated.evidence_refs.push_back(ev);
            }
        }
        if (!patch.superseded_by_knowledge_id.empty()) {
            updated.superseded_by_knowledge_id = patch.superseded_by_knowledge_id;
        }
        for (const auto& sid : patch.supersedes_knowledge_ids) {
            updated.supersedes_knowledge_ids.push_back(sid);
        }
        updated.updated_tick = input.revision_tick;
        for (const auto& rk : patch.reason_keys) {
            updated.reason_keys.push_back(rk);
        }

        // Update teaching_profile if status becomes Teachable
        if (updated.status == KnowledgeStatus::Teachable && !updated.teaching_profile.teachable) {
            updated.teaching_profile.teachable = true;
            updated.teaching_profile.teaching_message_key = "knowledge.teachable." + updated.subject.subject_id + "." + toString(updated.predicate.relation_type);
        }

        // Update projection_flags if status becomes Active/Teachable
        if (updated.status == KnowledgeStatus::Active || updated.status == KnowledgeStatus::Teachable) {
            updated.projection_flags.usable_for_action = true;
        }
        if (updated.status == KnowledgeStatus::Teachable) {
            updated.projection_flags.usable_for_teaching = true;
        }

        result.updated_claims.push_back(updated);
        result.applied_patches.push_back(patch);
    }

    // Add created claims
    for (auto& claim : plan.candidate_new_claims) {
        result.created_claims.push_back(claim);
    }

    // Determine decision
    switch (plan.assessment.recommended_strategy) {
        case KnowledgeResolutionStrategy::Reinforce:
            result.decision = KnowledgeRevisionDecision::ReinforcedClaim;
            break;
        case KnowledgeResolutionStrategy::Weaken:
            result.decision = KnowledgeRevisionDecision::WeakenedClaim;
            break;
        case KnowledgeResolutionStrategy::Specialize:
            result.decision = KnowledgeRevisionDecision::CreatedSpecializedClaim;
            break;
        case KnowledgeResolutionStrategy::Deprecate:
            result.decision = KnowledgeRevisionDecision::DeprecatedClaim;
            break;
        case KnowledgeResolutionStrategy::Disprove:
            result.decision = KnowledgeRevisionDecision::DisprovenClaim;
            break;
        default:
            result.decision = KnowledgeRevisionDecision::NoChange;
            break;
    }

    // Generate event drafts
    if (result.decision == KnowledgeRevisionDecision::ReinforcedClaim) {
        for (const auto& claim : result.updated_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_reinforced",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::UpdatedClaim,
                .reason_keys = {"knowledge.claim_reinforced"}
            });
        }
    } else if (result.decision == KnowledgeRevisionDecision::WeakenedClaim) {
        for (const auto& claim : result.updated_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_weakened",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::UpdatedClaim,
                .reason_keys = {"knowledge.claim_weakened"}
            });
        }
    } else if (result.decision == KnowledgeRevisionDecision::CreatedSpecializedClaim) {
        for (const auto& claim : result.created_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_specialized",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::CreatedClaim,
                .reason_keys = {"knowledge.claim_specialized"}
            });
        }
        for (const auto& claim : result.updated_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_deprecated",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::UpdatedClaim,
                .reason_keys = {"knowledge.claim_deprecated"}
            });
        }
    } else if (result.decision == KnowledgeRevisionDecision::DeprecatedClaim) {
        for (const auto& claim : result.updated_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_deprecated",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::UpdatedClaim,
                .reason_keys = {"knowledge.claim_deprecated"}
            });
        }
    } else if (result.decision == KnowledgeRevisionDecision::DisprovenClaim) {
        for (const auto& claim : result.updated_claims) {
            result.event_drafts.push_back(KnowledgeEventDraft{
                .event_key = "knowledge.claim_disproven",
                .knowledge_id = claim.knowledge_id,
                .owner = claim.owner,
                .subject = claim.subject,
                .relation_type = claim.predicate.relation_type,
                .status = claim.status,
                .decision = KnowledgeFormationDecision::UpdatedClaim,
                .reason_keys = {"knowledge.claim_disproven"}
            });
        }
    }

    // Generate state changes
    for (const auto& claim : result.updated_claims) {
        result.state_changes.push_back(KnowledgeStateChangeDraft{
            .change_key = "knowledge.state_change.updated",
            .knowledge_id = claim.knowledge_id,
            .reason_keys = {"knowledge_updated_by_revision"}
        });
    }
    for (const auto& claim : result.created_claims) {
        result.state_changes.push_back(KnowledgeStateChangeDraft{
            .change_key = "knowledge.state_change.created",
            .knowledge_id = claim.knowledge_id,
            .reason_keys = {"knowledge_created_by_revision"}
        });
    }

    result.plan = plan;
    result.reason_keys.push_back("revision_completed");

    // Final guard: all ok results must pass validateBasic
    auto final_validation = result.validateBasic();
    if (final_validation.is_error()) {
        return Result<KnowledgeRevisionResult>::fail(final_validation.errors());
    }

    return Result<KnowledgeRevisionResult>::ok(std::move(result));
}

// ============================================================
// KnowledgeRevisionApplier
// ============================================================

Result<std::vector<KnowledgeClaim>> KnowledgeRevisionApplier::applyToSnapshot(
    const std::vector<KnowledgeClaim>& current_claims,
    const KnowledgeRevisionResult& result) const {

    auto result_validation = result.validateBasic();
    if (result_validation.is_error()) {
        return Result<std::vector<KnowledgeClaim>>::fail(result_validation.errors());
    }

    std::vector<KnowledgeClaim> snapshot = current_claims;

    // Update existing claims
    for (const auto& updated : result.updated_claims) {
        bool found = false;
        for (auto& claim : snapshot) {
            if (claim.knowledge_id == updated.knowledge_id) {
                claim = updated;
                found = true;
                break;
            }
        }
        if (!found) {
            snapshot.push_back(updated);
        }
    }

    // Add created claims
    for (const auto& created : result.created_claims) {
        snapshot.push_back(created);
    }

    // Validate all output claims
    for (const auto& claim : snapshot) {
        auto r = claim.validateBasic();
        if (r.is_error()) {
            return Result<std::vector<KnowledgeClaim>>::fail(r.errors());
        }
    }

    return Result<std::vector<KnowledgeClaim>>::ok(std::move(snapshot));
}

} // namespace pathfinder::knowledge
