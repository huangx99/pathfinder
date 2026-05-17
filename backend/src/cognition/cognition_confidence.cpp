#include "pathfinder/cognition/cognition_confidence.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include <algorithm>
#include <cmath>

namespace pathfinder::cognition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ============================================================
// CognitionConfidenceModel
// ============================================================

CognitionConfidenceBand CognitionConfidenceModel::bandFor(double confidence, const CognitionConfidenceModelOptions& options) const {
    double c = std::clamp(confidence, options.min_confidence, options.max_confidence);
    if (c >= options.teachable_threshold) return CognitionConfidenceBand::Teachable;
    if (c >= options.reliable_threshold) return CognitionConfidenceBand::Reliable;
    if (c >= options.actionable_threshold) return CognitionConfidenceBand::Actionable;
    if (c >= 0.20) return CognitionConfidenceBand::Hypothesis;
    return CognitionConfidenceBand::Untrusted;
}

Result<CognitionUpdateResult> CognitionConfidenceModel::applyEvidence(
    const CognitionUpdateRequest& request) const {

    auto vr = request.validateBasic();
    if (vr.is_error()) {
        return Result<CognitionUpdateResult>::fail(vr.errors()[0]);
    }

    const auto& evidence = request.evidence;
    const auto& options = request.options;

    CognitionUpdateResult result;
    result.before_claim = request.existing_claim;

    CognitionClaimV2 claim;
    if (request.existing_claim) {
        claim = *request.existing_claim;
    } else {
        claim.key = evidence.key;
        claim.claim_id = makeClaimV2Id(
            evidence.key.subject.subject_id,
            evidence.key.target.target_id,
            evidence.key.action_id,
            evidence.key.aspect);
        claim.confidence = 0.0;
        claim.evidence_count = 0;
        claim.supporting_evidence_count = 0;
        claim.refuting_evidence_count = 0;
        claim.conflicted = false;
        claim.polarity = CognitionBeliefPolarity::Unknown;
    }

    // Update evidence tracking
    claim.evidence_count += 1;
    if (!claim.first_evidence_id) {
        claim.first_evidence_id = evidence.evidence_id;
    }
    claim.last_evidence_id = evidence.evidence_id;

    // Append reason/warning keys from evidence (avoid duplicates)
    for (const auto& key : evidence.reason_keys) {
        if (std::find(claim.reason_keys.begin(), claim.reason_keys.end(), key) == claim.reason_keys.end()) {
            claim.reason_keys.push_back(key);
        }
    }
    for (const auto& key : evidence.warning_keys) {
        if (std::find(claim.warning_keys.begin(), claim.warning_keys.end(), key) == claim.warning_keys.end()) {
            claim.warning_keys.push_back(key);
        }
    }

    // Apply confidence logic based on support
    double new_confidence = claim.confidence;
    switch (evidence.support) {
        case CognitionEvidenceSupport::Supports: {
            claim.supporting_evidence_count += 1;
            if (claim.polarity == CognitionBeliefPolarity::Unknown ||
                claim.polarity == CognitionBeliefPolarity::Positive) {
                claim.polarity = CognitionBeliefPolarity::Positive;
                if (claim.evidence_count == 1) {
                    new_confidence = options.initial_direct_feedback_confidence;
                } else {
                    new_confidence = claim.confidence + (options.initial_direct_feedback_confidence * options.reinforce_multiplier);
                }
            } else if (claim.polarity == CognitionBeliefPolarity::Negative) {
                // Supporting evidence against negative polarity creates conflict
                claim.conflicted = true;
                claim.polarity = CognitionBeliefPolarity::Mixed;
                new_confidence = std::max(0.0, claim.confidence - options.conflict_penalty);
            } else if (claim.polarity == CognitionBeliefPolarity::Mixed) {
                // Already mixed, slight recovery
                new_confidence = claim.confidence + (options.initial_direct_feedback_confidence * options.reinforce_multiplier * 0.5);
            }
            claim.risk_level = evidence.observed_risk;
            claim.utility_score = evidence.utility_signal;
            break;
        }
        case CognitionEvidenceSupport::Refutes: {
            claim.refuting_evidence_count += 1;
            if (claim.polarity == CognitionBeliefPolarity::Unknown) {
                claim.polarity = CognitionBeliefPolarity::Negative;
                new_confidence = options.initial_direct_feedback_confidence * options.refute_multiplier;
            } else if (claim.polarity == CognitionBeliefPolarity::Positive) {
                // Refute positive claim
                new_confidence = claim.confidence - (options.initial_direct_feedback_confidence * options.refute_multiplier);
                if (new_confidence < options.actionable_threshold) {
                    claim.conflicted = true;
                    claim.polarity = CognitionBeliefPolarity::Mixed;
                }
            } else if (claim.polarity == CognitionBeliefPolarity::Negative) {
                // Refute negative claim -> strengthens negative? No, refute means opposite of current polarity.
                // For negative polarity, refute means "it's not negative", so weaken.
                new_confidence = claim.confidence - (options.initial_direct_feedback_confidence * options.refute_multiplier);
            } else if (claim.polarity == CognitionBeliefPolarity::Mixed) {
                new_confidence = claim.confidence - (options.initial_direct_feedback_confidence * options.refute_multiplier * 0.5);
            }
            break;
        }
        case CognitionEvidenceSupport::Conflicts: {
            claim.conflicted = true;
            claim.polarity = CognitionBeliefPolarity::Mixed;
            new_confidence = std::max(0.0, claim.confidence - options.conflict_penalty);
            break;
        }
        case CognitionEvidenceSupport::Neutral: {
            // Neutral doesn't significantly change confidence
            new_confidence = claim.confidence;
            break;
        }
        case CognitionEvidenceSupport::Unknown:
            return Result<CognitionUpdateResult>::fail(
                makeError(ErrorCode::validation_failed, "CognitionConfidenceModel: evidence support is Unknown"));
    }

    // Clamp confidence
    claim.confidence = std::clamp(new_confidence, options.min_confidence, options.max_confidence);
    claim.confidence_band = bandFor(claim.confidence, options);

    // Determine decision
    if (!request.existing_claim) {
        result.decision = CognitionUpdateDecision::Created;
    } else if (evidence.support == CognitionEvidenceSupport::Conflicts) {
        result.decision = CognitionUpdateDecision::Conflicted;
    } else if (evidence.support == CognitionEvidenceSupport::Refutes && claim.confidence < request.existing_claim->confidence) {
        result.decision = CognitionUpdateDecision::Weakened;
    } else if (evidence.support == CognitionEvidenceSupport::Supports && claim.confidence > request.existing_claim->confidence) {
        result.decision = CognitionUpdateDecision::Reinforced;
    } else if (evidence.support == CognitionEvidenceSupport::Neutral) {
        result.decision = CognitionUpdateDecision::Ignored;
    } else {
        result.decision = CognitionUpdateDecision::Reinforced;
    }

    result.after_claim = std::move(claim);
    return Result<CognitionUpdateResult>::ok(std::move(result));
}

// ============================================================
// CognitionUpdaterV2
// ============================================================

Result<CognitionUpdateResult> CognitionUpdaterV2::applyEvidence(
    CognitionStateV2& state,
    const CognitionEvidenceRecord& evidence,
    const CognitionConfidenceModelOptions& options) const {

    // Validate evidence first
    auto validation = evidence.validateBasic();
    if (validation.is_error()) {
        return Result<CognitionUpdateResult>::fail(validation.errors()[0]);
    }

    // Validate options
    auto opt_validation = options.validateBasic();
    if (opt_validation.is_error()) {
        return Result<CognitionUpdateResult>::fail(opt_validation.errors()[0]);
    }

    // Find existing claim
    const auto* existing = state.findClaim(evidence.key);

    // Build request
    CognitionUpdateRequest request;
    if (existing) {
        request.existing_claim = *existing;
    }
    request.evidence = evidence;
    request.options = options;

    // Apply confidence model
    CognitionConfidenceModel model;
    auto result = model.applyEvidence(request);
    if (result.is_error()) {
        return result;
    }

    // Only after successful computation, write to state
    auto append_result = state.appendEvidence(evidence);
    if (append_result.is_error()) {
        return Result<CognitionUpdateResult>::fail(append_result.errors()[0]);
    }

    auto upsert_result = state.upsertClaim(result.value().after_claim);
    if (upsert_result.is_error()) {
        return Result<CognitionUpdateResult>::fail(upsert_result.errors()[0]);
    }

    return result;
}

} // namespace pathfinder::cognition
