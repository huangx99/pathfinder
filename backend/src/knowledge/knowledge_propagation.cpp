#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/condition/condition_normalizer.h"
#include <algorithm>
#include <sstream>

namespace pathfinder::knowledge {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::KnowledgeId;

// ============================================================
// Helpers
// ============================================================

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static double getChannelWeight(KnowledgePropagationChannel channel, const KnowledgePropagationOptions& options) {
    switch (channel) {
        case KnowledgePropagationChannel::DirectTeaching: return options.direct_teaching_weight;
        case KnowledgePropagationChannel::Demonstration: return options.demonstration_weight;
        case KnowledgePropagationChannel::WarningSignal: return options.warning_weight;
        case KnowledgePropagationChannel::SharedObservation: return options.shared_observation_weight;
        case KnowledgePropagationChannel::TribeInstruction: return options.tribe_instruction_weight;
        case KnowledgePropagationChannel::Correction: return options.correction_weight;
        default: return 0.5;
    }
}

static double getDefaultTrustScore(KnowledgePropagationTrustBand band) {
    switch (band) {
        case KnowledgePropagationTrustBand::Distrusted: return 0.10;
        case KnowledgePropagationTrustBand::Low: return 0.30;
        case KnowledgePropagationTrustBand::Medium: return 0.55;
        case KnowledgePropagationTrustBand::High: return 0.80;
        case KnowledgePropagationTrustBand::Authority: return 0.90;
        default: return 0.5;
    }
}

static KnowledgeStatus confidenceToStatus(double confidence) {
    if (confidence < 0.10) return KnowledgeStatus::Candidate;
    if (confidence < 0.35) return KnowledgeStatus::Hypothesis;
    if (confidence < 0.70) return KnowledgeStatus::Shared;
    return KnowledgeStatus::Active;
}

static bool isClaimTransferableByDefault(KnowledgeStatus status) {
    return status == KnowledgeStatus::Active
        || status == KnowledgeStatus::Teachable
        || status == KnowledgeStatus::Shared
        || status == KnowledgeStatus::Operational;
}

static std::vector<std::string> canonicalConditionKeys(const std::vector<KnowledgeCondition>& conditions) {
    std::vector<std::string> keys;
    keys.reserve(conditions.size());
    for (const auto& condition : conditions) {
        auto canonical = canonicalKnowledgeConditionKey(condition);
        if (canonical.is_error()) return {};
        keys.push_back(canonical.value());
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

static bool claimsShareConditionScope(const KnowledgeClaim& source, const KnowledgeClaim& target) {
    return canonicalConditionKeys(source.conditions) == canonicalConditionKeys(target.conditions);
}

static bool claimsMatchForReinforcement(const KnowledgeClaim& source, const KnowledgeClaim& target) {
    if (source.subject.subject_id != target.subject.subject_id) return false;
    if (source.subject.related_subject_ids != target.subject.related_subject_ids) return false;
    if (source.subject.relation_group_key != target.subject.relation_group_key) return false;
    if (source.predicate.relation_type != target.predicate.relation_type) return false;
    if (source.predicate.action_key != target.predicate.action_key) return false;
    if (source.predicate.effect_key != target.predicate.effect_key) return false;
    if (!claimsShareConditionScope(source, target)) return false;
    return true;
}

static bool claimsMatchForConflictCheck(const KnowledgeClaim& source, const KnowledgeClaim& target) {
    if (source.subject.subject_id != target.subject.subject_id) return false;
    if (source.subject.related_subject_ids != target.subject.related_subject_ids) return false;
    if (source.subject.relation_group_key != target.subject.relation_group_key) return false;
    if (source.predicate.relation_type != target.predicate.relation_type) return false;
    if (source.predicate.action_key != target.predicate.action_key) return false;
    if (!claimsShareConditionScope(source, target)) return false;
    return true;
}

static bool isSameOwner(const KnowledgeOwner& a, const KnowledgeOwner& b) {
    if (a.kind != b.kind) return false;
    if (!a.entity_id.empty() && a.entity_id != b.entity_id) return false;
    if (!a.tribe_id.empty() && a.tribe_id != b.tribe_id) return false;
    if (!a.region_id.empty() && a.region_id != b.region_id) return false;
    if (!a.external_key.empty() && a.external_key != b.external_key) return false;
    return true;
}

// ============================================================
// KnowledgePropagationOptions
// ============================================================

Result<void> KnowledgePropagationOptions::validateBasic() const {
    auto check01 = [&](const char* name, double v) -> Result<void> {
        if (v < 0.0 || v > 1.0) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed,
                std::string("KnowledgePropagationOptions ") + name + " out of range [0,1]"));
        }
        return Result<void>::ok();
    };
    auto r = check01("min_source_confidence", min_source_confidence); if (r.is_error()) return r;
    r = check01("min_transfer_score", min_transfer_score); if (r.is_error()) return r;
    r = check01("direct_teaching_weight", direct_teaching_weight); if (r.is_error()) return r;
    r = check01("demonstration_weight", demonstration_weight); if (r.is_error()) return r;
    r = check01("warning_weight", warning_weight); if (r.is_error()) return r;
    r = check01("shared_observation_weight", shared_observation_weight); if (r.is_error()) return r;
    r = check01("tribe_instruction_weight", tribe_instruction_weight); if (r.is_error()) return r;
    r = check01("correction_weight", correction_weight); if (r.is_error()) return r;
    r = check01("max_created_confidence", max_created_confidence); if (r.is_error()) return r;
    r = check01("max_shared_confidence", max_shared_confidence); if (r.is_error()) return r;
    if (max_claims_per_attempt == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationOptions max_claims_per_attempt cannot be 0"));
    }
    if (max_evidence_refs == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationOptions max_evidence_refs cannot be 0"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagator
// ============================================================

Result<void> KnowledgePropagator::validateBasic() const {
    auto r = owner.validateBasic();
    if (r.is_error()) return r;
    if (containsKnowledgeForbiddenKey(capability_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagator capability_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagator reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationTarget
// ============================================================

Result<void> KnowledgePropagationTarget::validateBasic() const {
    auto r = owner.validateBasic();
    if (r.is_error()) return r;
    if (containsKnowledgeForbiddenKey(capability_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationTarget capability_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationTarget reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationContext
// ============================================================

Result<void> KnowledgePropagationContext::validateBasic() const {
    if (channel == KnowledgePropagationChannel::Unknown || channel == KnowledgePropagationChannel::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationContext channel is Unknown or TestOnly"));
    }
    if (trust_band == KnowledgePropagationTrustBand::Unknown || trust_band == KnowledgePropagationTrustBand::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationContext trust_band is Unknown or TestOnly"));
    }
    auto check01 = [&](const char* name, double v) -> Result<void> {
        if (v < 0.0 || v > 1.0) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed,
                std::string("KnowledgePropagationContext ") + name + " out of range [0,1]"));
        }
        return Result<void>::ok();
    };
    auto r = check01("trust_score", trust_score); if (r.is_error()) return r;
    r = check01("distance_factor", distance_factor); if (r.is_error()) return r;
    r = check01("channel_quality", channel_quality); if (r.is_error()) return r;
    r = check01("noise_factor", noise_factor); if (r.is_error()) return r;
    if (propagation_tick.value() < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationContext propagation_tick invalid"));
    }
    if (containsKnowledgeForbiddenKey(condition_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationContext condition_keys contain forbidden key"));
    }
    if (!condition_keys.empty()) {
        pathfinder::condition::ConditionNormalizer normalizer;
        auto normalized = normalizer.normalizeKeys(condition_keys);
        if (normalized.is_error()) return Result<void>::fail(normalized.errors());
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationContext reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationSourceClaim
// ============================================================

Result<void> KnowledgePropagationSourceClaim::validateBasic() const {
    auto r = claim.validateBasic();
    if (r.is_error()) return r;
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationSourceClaim reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationAttempt
// ============================================================

Result<void> KnowledgePropagationAttempt::validateBasic() const {
    if (attempt_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationAttempt attempt_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(attempt_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationAttempt attempt_key contains forbidden key"));
    }
    auto r = propagator.validateBasic();
    if (r.is_error()) return r;
    r = target.validateBasic();
    if (r.is_error()) return r;
    r = context.validateBasic();
    if (r.is_error()) return r;
    for (const auto& sc : source_claims) {
        r = sc.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& tc : target_existing_claims) {
        r = tc.validateBasic();
        if (r.is_error()) return r;
        if (!isSameOwner(tc.owner, target.owner)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed,
                "KnowledgePropagationAttempt target_existing_claim owner mismatch"));
        }
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationAttempt reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeTransferAssessment
// ============================================================

Result<void> KnowledgeTransferAssessment::validateBasic() const {
    if (source_knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment source_knowledge_id is empty"));
    }
    if (recommended_decision == KnowledgePropagationDecision::Unknown ||
        recommended_decision == KnowledgePropagationDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment recommended_decision is Unknown or TestOnly"));
    }
    if (transfer_score < 0.0 || transfer_score > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment transfer_score out of range [0,1]"));
    }
    if (created_confidence < 0.0 || created_confidence > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment created_confidence out of range [0,1]"));
    }
    if (!can_transfer && failure_reason == KnowledgePropagationFailureReason::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment can_transfer=false requires failure_reason"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeTransferAssessment warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgeRecipientClaimDraft
// ============================================================

Result<void> KnowledgeRecipientClaimDraft::validateBasic() const {
    auto r = claim.validateBasic();
    if (r.is_error()) return r;
    if (source_knowledge_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeRecipientClaimDraft source_knowledge_id is empty"));
    }
    r = source_owner.validateBasic();
    if (r.is_error()) return r;
    if (channel == KnowledgePropagationChannel::Unknown || channel == KnowledgePropagationChannel::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeRecipientClaimDraft channel is Unknown or TestOnly"));
    }
    if (transfer_score < 0.0 || transfer_score > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeRecipientClaimDraft transfer_score out of range [0,1]"));
    }
    bool has_teaching = false;
    for (const auto& ev : claim.evidence_refs) {
        if (ev.evidence_kind == KnowledgeEvidenceKind::Teaching) {
            has_teaching = true;
            break;
        }
    }
    if (!has_teaching) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeRecipientClaimDraft claim must contain Teaching evidence"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgeRecipientClaimDraft reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationPlan
// ============================================================

Result<void> KnowledgePropagationPlan::validateBasic() const {
    if (plan_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationPlan plan_key is empty"));
    }
    if (containsKnowledgeForbiddenKey(plan_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationPlan plan_key contains forbidden key"));
    }
    auto r = attempt.validateBasic();
    if (r.is_error()) return r;
    for (const auto& a : assessments) {
        r = a.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& d : recipient_claim_drafts) {
        r = d.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& p : target_revision_patches) {
        r = p.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationPlan reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationPlan warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationResult
// ============================================================

bool KnowledgePropagationResult::ok() const {
    return decision == KnowledgePropagationDecision::Skipped ||
           decision == KnowledgePropagationDecision::Failed ||
           decision == KnowledgePropagationDecision::CreatedRecipientClaim ||
           decision == KnowledgePropagationDecision::UpdatedRecipientClaim ||
           decision == KnowledgePropagationDecision::ReinforcedRecipientClaim ||
           decision == KnowledgePropagationDecision::WeakenedRecipientClaim ||
           decision == KnowledgePropagationDecision::CorrectionDelivered;
}

Result<void> KnowledgePropagationResult::validateBasic() const {
    if (decision == KnowledgePropagationDecision::Unknown ||
        decision == KnowledgePropagationDecision::TestOnly ||
        decision == KnowledgePropagationDecision::Rejected) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationResult decision is Unknown, TestOnly or Rejected"));
    }
    if (plan.has_value()) {
        auto r = plan->validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& c : created_claims) {
        auto r = c.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& c : updated_claims) {
        auto r = c.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& p : applied_patches) {
        auto r = p.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& e : event_drafts) {
        auto r = e.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& s : state_changes) {
        auto r = s.validateBasic();
        if (r.is_error()) return r;
    }
    if (decision == KnowledgePropagationDecision::CreatedRecipientClaim && created_claims.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationResult CreatedRecipientClaim requires created_claims"));
    }
    if (containsKnowledgeForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationResult reason_keys contain forbidden key"));
    }
    if (containsKnowledgeForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed,
            "KnowledgePropagationResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

// ============================================================
// KnowledgePropagationPlanner
// ============================================================

Result<KnowledgePropagationPlan> KnowledgePropagationPlanner::planPropagation(
    const KnowledgePropagationAttempt& attempt,
    const KnowledgePropagationOptions& options) const {

    // Validate inputs
    auto opts_r = options.validateBasic();
    if (opts_r.is_error()) {
        return Result<KnowledgePropagationPlan>::fail(opts_r.errors());
    }
    auto attempt_r = attempt.validateBasic();
    if (attempt_r.is_error()) {
        return Result<KnowledgePropagationPlan>::fail(attempt_r.errors());
    }

    // Security: owner consistency
    for (const auto& sc : attempt.source_claims) {
        if (!isSameOwner(sc.claim.owner, attempt.propagator.owner)) {
            return Result<KnowledgePropagationPlan>::fail(
                makeError(ErrorCode::validation_failed, "Source claim owner mismatch with propagator"));
        }
    }

    // Same owner check
    if (!options.allow_same_owner && isSameOwner(attempt.propagator.owner, attempt.target.owner)) {
        return Result<KnowledgePropagationPlan>::fail(
            makeError(ErrorCode::validation_failed, "Same owner propagation not allowed"));
    }

    // Direction checks
    if (attempt.propagator.owner.kind == KnowledgeOwnerKind::Agent && attempt.target.owner.kind == KnowledgeOwnerKind::Agent) {
        if (!options.allow_agent_to_agent) {
            return Result<KnowledgePropagationPlan>::fail(
                makeError(ErrorCode::validation_failed, "Agent-to-agent propagation not allowed"));
        }
    }
    if (attempt.propagator.owner.kind == KnowledgeOwnerKind::Agent && attempt.target.owner.kind == KnowledgeOwnerKind::Tribe) {
        if (!options.allow_agent_to_tribe) {
            return Result<KnowledgePropagationPlan>::fail(
                makeError(ErrorCode::validation_failed, "Agent-to-tribe propagation not allowed"));
        }
    }
    if (attempt.propagator.owner.kind == KnowledgeOwnerKind::Tribe && attempt.target.owner.kind == KnowledgeOwnerKind::Agent) {
        if (!options.allow_tribe_to_agent) {
            return Result<KnowledgePropagationPlan>::fail(
                makeError(ErrorCode::validation_failed, "Tribe-to-agent propagation not allowed"));
        }
    }

    KnowledgePropagationPlan plan;
    plan.plan_key = attempt.attempt_key + "_plan";
    plan.attempt = attempt;

    // If no source claims selected
    bool has_selected = false;
    for (const auto& sc : attempt.source_claims) {
        if (sc.selected) { has_selected = true; break; }
    }
    if (!has_selected) {
        KnowledgeTransferAssessment assess;
        assess.source_knowledge_id = KnowledgeId("skipped_no_source");
        assess.recommended_decision = KnowledgePropagationDecision::Skipped;
        assess.can_transfer = false;
        assess.failure_reason = KnowledgePropagationFailureReason::NoTransferableClaim;
        plan.assessments.push_back(assess);
        auto plan_r = plan.validateBasic();
        if (plan_r.is_error()) {
            return Result<KnowledgePropagationPlan>::fail(plan_r.errors());
        }
        return Result<KnowledgePropagationPlan>::ok(plan);
    }

    // Process each source claim
    double effective_trust = attempt.context.trust_score;
    if (effective_trust <= 0.0 || effective_trust > 1.0) {
        effective_trust = getDefaultTrustScore(attempt.context.trust_band);
    }
    double channel_weight = getChannelWeight(attempt.context.channel, options);
    size_t processed_count = 0;

    for (const auto& sc : attempt.source_claims) {
        if (!sc.selected) continue;

        // max_claims_per_attempt limit
        if (processed_count >= options.max_claims_per_attempt) {
            KnowledgeTransferAssessment assess;
            assess.source_knowledge_id = sc.claim.knowledge_id;
            assess.recommended_decision = KnowledgePropagationDecision::Skipped;
            assess.can_transfer = false;
            assess.failure_reason = KnowledgePropagationFailureReason::NoTransferableClaim;
            assess.reason_keys.push_back("max_claims_per_attempt_reached");
            plan.assessments.push_back(assess);
            continue;
        }
        ++processed_count;

        const auto& claim = sc.claim;
        KnowledgeTransferAssessment assess;
        assess.source_knowledge_id = claim.knowledge_id;

        // Check transferable status
        bool is_deprecated = claim.status == KnowledgeStatus::Deprecated;
        bool is_disproven = claim.status == KnowledgeStatus::Disproven;

        if (is_deprecated && !options.allow_deprecated) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ClaimDeprecated;
            assess.can_transfer = false;
            plan.assessments.push_back(assess);
            continue;
        }
        if (is_disproven && !options.allow_disproven) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ClaimDisproven;
            assess.can_transfer = false;
            plan.assessments.push_back(assess);
            continue;
        }

        if (!isClaimTransferableByDefault(claim.status) && !is_deprecated && !is_disproven) {
            // Candidate / Hypothesis can still propagate with penalty
            if (claim.status != KnowledgeStatus::Candidate && claim.status != KnowledgeStatus::Hypothesis) {
                assess.recommended_decision = KnowledgePropagationDecision::Failed;
                assess.failure_reason = KnowledgePropagationFailureReason::ClaimNotTeachable;
                assess.can_transfer = false;
                plan.assessments.push_back(assess);
                continue;
            }
        }

        double source_confidence = claim.confidence.confidence;
        if (source_confidence < options.min_source_confidence) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ClaimNotTeachable;
            assess.can_transfer = false;
            plan.assessments.push_back(assess);
            continue;
        }

        // Compute transfer score
        double noise_penalty = 1.0 - attempt.context.noise_factor;
        double transfer_score = clamp01(
            source_confidence * channel_weight * effective_trust *
            attempt.context.distance_factor * attempt.context.channel_quality * noise_penalty);

        if (transfer_score < options.min_transfer_score) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ChannelTooWeak;
            assess.transfer_score = transfer_score;
            assess.can_transfer = false;
            plan.assessments.push_back(assess);
            continue;
        }

        double created_confidence = std::min(transfer_score, options.max_created_confidence);

        // Candidate / Hypothesis penalty
        if (claim.status == KnowledgeStatus::Candidate || claim.status == KnowledgeStatus::Hypothesis) {
            created_confidence *= 0.75;
        }

        // WarningSignal penalty for non-risk claims
        if (attempt.context.channel == KnowledgePropagationChannel::WarningSignal) {
            if (claim.predicate.relation_type != KnowledgeRelationType::HasRisk &&
                claim.predicate.relation_type != KnowledgeRelationType::IsDangerousUnder) {
                created_confidence *= 0.8;
            }
        }

        // Correction channel cap
        if (attempt.context.channel == KnowledgePropagationChannel::Correction) {
            created_confidence = std::min(transfer_score, options.correction_weight);
        }

        // max_shared_confidence cap for Shared status
        if (confidenceToStatus(created_confidence) == KnowledgeStatus::Shared) {
            created_confidence = std::min(created_confidence, options.max_shared_confidence);
        }

        created_confidence = clamp01(created_confidence);
        assess.transfer_score = transfer_score;
        assess.created_confidence = created_confidence;
        assess.can_transfer = true;

        // Match against target existing claims
        const KnowledgeClaim* exact_match_target = nullptr;
        const KnowledgeClaim* conflict_target = nullptr;
        for (const auto& tc : attempt.target_existing_claims) {
            if (claimsMatchForReinforcement(claim, tc)) {
                exact_match_target = &tc;
                assess.matched_target_claim_ids.push_back(tc.knowledge_id);
            } else if (claimsMatchForConflictCheck(claim, tc)) {
                conflict_target = &tc;
                assess.matched_target_claim_ids.push_back(tc.knowledge_id);
            }
        }

        if (exact_match_target != nullptr) {
            assess.recommended_decision = KnowledgePropagationDecision::ReinforcedRecipientClaim;
            assess.should_update_existing = true;
        } else if (conflict_target != nullptr) {
            assess.recommended_decision = KnowledgePropagationDecision::WeakenedRecipientClaim;
            assess.should_route_to_revision = true;
        } else {
            assess.recommended_decision = KnowledgePropagationDecision::CreatedRecipientClaim;
            assess.should_create_claim = true;
        }

        // Check options restrictions
        if (assess.should_create_claim && !options.allow_create_recipient_claim) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ClaimNotTeachable;
            assess.can_transfer = false;
            assess.should_create_claim = false;
            plan.assessments.push_back(assess);
            continue;
        }
        if (assess.should_update_existing && !options.allow_update_recipient_claim) {
            assess.recommended_decision = KnowledgePropagationDecision::Failed;
            assess.failure_reason = KnowledgePropagationFailureReason::ClaimNotTeachable;
            assess.can_transfer = false;
            assess.should_update_existing = false;
            plan.assessments.push_back(assess);
            continue;
        }

        // Generate recipient claim draft if applicable
        if (assess.should_create_claim) {
            KnowledgeRecipientClaimDraft draft;
            draft.source_knowledge_id = claim.knowledge_id;
            draft.source_owner = claim.owner;
            draft.channel = attempt.context.channel;
            draft.transfer_score = assess.transfer_score;
            const auto draft_sequence_index = plan.recipient_claim_drafts.size();

            // Build new recipient claim
            KnowledgeClaim recipient_claim;
            KnowledgeIdFactory id_factory;
            auto kid_r = id_factory.makeKnowledgeId(attempt.target.owner, claim.subject, claim.predicate,
                                                     attempt.context.propagation_tick, draft_sequence_index);
            if (kid_r.is_ok()) {
                recipient_claim.knowledge_id = kid_r.value();
            } else {
                std::string fallback_id = "know_prop_" + attempt.target.owner.entity_id.value() + "_" +
                                          claim.subject.subject_id + "_" + toString(claim.predicate.relation_type) +
                                          "_" + std::to_string(attempt.context.propagation_tick.value()) + "_" +
                                          std::to_string(draft_sequence_index);
                recipient_claim.knowledge_id = KnowledgeId(fallback_id);
            }
            recipient_claim.owner = attempt.target.owner;
            recipient_claim.subject = claim.subject;
            recipient_claim.predicate = claim.predicate;
            recipient_claim.conditions = claim.conditions;
            recipient_claim.confidence.confidence = assess.created_confidence;
            recipient_claim.confidence.band = confidenceToBand(assess.created_confidence);
            recipient_claim.confidence.support_count = 1;
            recipient_claim.status = confidenceToStatus(assess.created_confidence);
            recipient_claim.created_tick = attempt.context.propagation_tick;
            recipient_claim.updated_tick = attempt.context.propagation_tick;
            recipient_claim.related_knowledge_ids.push_back(claim.knowledge_id);
            recipient_claim.reason_keys.push_back("received_by_propagation");
            recipient_claim.reason_keys.push_back("propagated_from_source");

            // Teaching evidence
            KnowledgeEvidence teaching_ev;
            teaching_ev.evidence_id = KnowledgeEvidenceId("kevd_prop_" + claim.knowledge_id.value() + "_0");
            teaching_ev.evidence_kind = KnowledgeEvidenceKind::Teaching;
            teaching_ev.source_summary_id = pathfinder::memory::MemorySummaryId("prop_sum_" + claim.knowledge_id.value());
            teaching_ev.supports_claim = true;
            teaching_ev.reliability = assess.transfer_score;
            teaching_ev.confidence_delta = assess.created_confidence;
            teaching_ev.observed_tick = attempt.context.propagation_tick;
            teaching_ev.reason_keys.push_back("teaching_from_propagation");
            recipient_claim.evidence_refs.push_back(teaching_ev);

            draft.claim = recipient_claim;
            plan.recipient_claim_drafts.push_back(draft);
        }

        if (assess.should_update_existing && exact_match_target != nullptr) {
            // Build updated recipient claim based on existing target claim
            KnowledgeRecipientClaimDraft draft;
            draft.source_knowledge_id = claim.knowledge_id;
            draft.source_owner = claim.owner;
            draft.channel = attempt.context.channel;
            draft.transfer_score = assess.transfer_score;

            KnowledgeClaim recipient_claim = *exact_match_target;
            // Update confidence: reinforcement must never lower an existing stronger claim.
            double new_confidence = std::max(
                recipient_claim.confidence.confidence,
                std::min(recipient_claim.confidence.confidence + assess.created_confidence * 0.25, 1.0));
            recipient_claim.confidence.confidence = new_confidence;
            recipient_claim.confidence.band = confidenceToBand(new_confidence);
            recipient_claim.confidence.support_count += 1;
            recipient_claim.status = confidenceToStatus(new_confidence);
            recipient_claim.updated_tick = attempt.context.propagation_tick;
            recipient_claim.related_knowledge_ids.push_back(claim.knowledge_id);
            recipient_claim.reason_keys.push_back("reinforced_by_propagation");

            // Append Teaching evidence
            KnowledgeEvidence teaching_ev;
            teaching_ev.evidence_id = KnowledgeEvidenceId("kevd_prop_" + claim.knowledge_id.value() + "_0");
            teaching_ev.evidence_kind = KnowledgeEvidenceKind::Teaching;
            teaching_ev.source_summary_id = pathfinder::memory::MemorySummaryId("prop_sum_" + claim.knowledge_id.value());
            teaching_ev.supports_claim = true;
            teaching_ev.reliability = assess.transfer_score;
            teaching_ev.confidence_delta = assess.created_confidence;
            teaching_ev.observed_tick = attempt.context.propagation_tick;
            teaching_ev.reason_keys.push_back("teaching_reinforcement");
            recipient_claim.evidence_refs.push_back(teaching_ev);

            draft.claim = recipient_claim;
            plan.recipient_claim_drafts.push_back(draft);
        }

        // P20 first version: only set should_route_to_revision, do not generate invalid patches
        // P21 or caller will explicitly invoke P19 revision service
        if (assess.should_route_to_revision) {
            // No patch generation in P20; route signal only
        }

        plan.assessments.push_back(assess);
    }

    // If no assessments were produced (all filtered out)
    if (plan.assessments.empty()) {
        KnowledgeTransferAssessment assess;
        assess.source_knowledge_id = KnowledgeId("skipped_no_claims");
        assess.recommended_decision = KnowledgePropagationDecision::Skipped;
        assess.can_transfer = false;
        assess.failure_reason = KnowledgePropagationFailureReason::NoTransferableClaim;
        plan.assessments.push_back(assess);
    }

    auto plan_r = plan.validateBasic();
    if (plan_r.is_error()) {
        return Result<KnowledgePropagationPlan>::fail(plan_r.errors());
    }
    return Result<KnowledgePropagationPlan>::ok(plan);
}

// ============================================================
// KnowledgePropagationService
// ============================================================

Result<KnowledgePropagationResult> KnowledgePropagationService::propagate(
    const KnowledgePropagationAttempt& attempt,
    const KnowledgePropagationOptions& options) const {

    KnowledgePropagationPlanner planner;
    auto plan_r = planner.planPropagation(attempt, options);
    if (plan_r.is_error()) {
        return Result<KnowledgePropagationResult>::fail(plan_r.errors());
    }

    auto plan = plan_r.value();
    KnowledgePropagationResult result;
    result.plan = plan;

    // Determine overall decision from assessments
    bool any_created = false;
    bool any_reinforced = false;
    bool any_weakened = false;
    bool any_correction = false;
    bool any_failed = false;

    for (const auto& assess : plan.assessments) {
        switch (assess.recommended_decision) {
            case KnowledgePropagationDecision::CreatedRecipientClaim:
                any_created = true;
                break;
            case KnowledgePropagationDecision::ReinforcedRecipientClaim:
            case KnowledgePropagationDecision::UpdatedRecipientClaim:
                any_reinforced = true;
                break;
            case KnowledgePropagationDecision::WeakenedRecipientClaim:
                any_weakened = true;
                break;
            case KnowledgePropagationDecision::CorrectionDelivered:
                any_correction = true;
                break;
            case KnowledgePropagationDecision::Failed:
                any_failed = true;
                break;
            default:
                break;
        }
    }

    if (any_correction) {
        result.decision = KnowledgePropagationDecision::CorrectionDelivered;
    } else if (any_created) {
        result.decision = KnowledgePropagationDecision::CreatedRecipientClaim;
    } else if (any_reinforced) {
        result.decision = KnowledgePropagationDecision::ReinforcedRecipientClaim;
    } else if (any_weakened) {
        result.decision = KnowledgePropagationDecision::WeakenedRecipientClaim;
    } else if (any_failed) {
        result.decision = KnowledgePropagationDecision::Failed;
    } else {
        result.decision = KnowledgePropagationDecision::Skipped;
    }

    auto matches_existing_target = [&](const KnowledgeClaim& claim) {
        return std::any_of(
            plan.attempt.target_existing_claims.begin(),
            plan.attempt.target_existing_claims.end(),
            [&](const KnowledgeClaim& existing) { return existing.knowledge_id == claim.knowledge_id; });
    };

    // Convert drafts by per-claim destination, not by the aggregate decision.
    // A single teaching attempt may both create a new claim and reinforce an existing one.
    for (const auto& draft : plan.recipient_claim_drafts) {
        if (matches_existing_target(draft.claim)) {
            result.updated_claims.push_back(draft.claim);
        } else {
            result.created_claims.push_back(draft.claim);
        }
    }

    // Apply patches
    for (const auto& patch : plan.target_revision_patches) {
        result.applied_patches.push_back(patch);
    }

    // Generate event drafts for successful transfers
    for (const auto& claim : result.created_claims) {
        KnowledgeEventDraft evt;
        evt.event_key = "knowledge.claim_transferred";
        evt.knowledge_id = claim.knowledge_id;
        evt.owner = claim.owner;
        evt.subject = claim.subject;
        evt.relation_type = claim.predicate.relation_type;
        evt.status = claim.status;
        evt.decision = KnowledgeFormationDecision::CreatedClaim;
        evt.reason_keys.push_back("propagation_transfer");
        result.event_drafts.push_back(evt);

        KnowledgeStateChangeDraft sc;
        sc.change_key = "knowledge.state_change.propagation_created";
        sc.knowledge_id = claim.knowledge_id;
        sc.reason_keys.push_back("propagation");
        result.state_changes.push_back(sc);
    }

    for (const auto& claim : result.updated_claims) {
        KnowledgeEventDraft evt;
        evt.event_key = "knowledge.claim_transfer_reinforced";
        evt.knowledge_id = claim.knowledge_id;
        evt.owner = claim.owner;
        evt.subject = claim.subject;
        evt.relation_type = claim.predicate.relation_type;
        evt.status = claim.status;
        evt.decision = KnowledgeFormationDecision::UpdatedClaim;
        evt.reason_keys.push_back("propagation_reinforcement");
        result.event_drafts.push_back(evt);

        KnowledgeStateChangeDraft sc;
        sc.change_key = "knowledge.state_change.propagation_updated";
        sc.knowledge_id = claim.knowledge_id;
        sc.reason_keys.push_back("propagation");
        result.state_changes.push_back(sc);
    }

    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<KnowledgePropagationResult>::fail(result_r.errors());
    }
    return Result<KnowledgePropagationResult>::ok(result);
}

// ============================================================
// KnowledgePropagationApplier
// ============================================================

Result<std::vector<KnowledgeClaim>> KnowledgePropagationApplier::applyToTargetSnapshot(
    const std::vector<KnowledgeClaim>& current_target_claims,
    const KnowledgePropagationResult& result) const {

    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<std::vector<KnowledgeClaim>>::fail(result_r.errors());
    }

    std::vector<KnowledgeClaim> snapshot = current_target_claims;

    // Add created claims
    for (const auto& c : result.created_claims) {
        snapshot.push_back(c);
    }

    // Simple update: replace matching claims with updated version
    for (const auto& updated : result.updated_claims) {
        bool replaced = false;
        for (auto& existing : snapshot) {
            if (existing.knowledge_id == updated.knowledge_id) {
                existing = updated;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            // If not found, append as new
            snapshot.push_back(updated);
        }
    }

    for (const auto& c : snapshot) {
        auto r = c.validateBasic();
        if (r.is_error()) {
            return Result<std::vector<KnowledgeClaim>>::fail(r.errors());
        }
    }

    return Result<std::vector<KnowledgeClaim>>::ok(snapshot);
}

} // namespace pathfinder::knowledge
