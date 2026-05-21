#include "pathfinder/world_teaching/world_teaching_loop_bridge.h"
#include "pathfinder/knowledge/knowledge_propagation.h"
#include "pathfinder/foundation/error.h"

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace pathfinder::world_teaching {

WorldTeachingLoopBridge::BridgeResult WorldTeachingLoopBridge::propagate(
    const pathfinder::knowledge::KnowledgeClaim& source_claim,
    const pathfinder::knowledge::KnowledgeOwner& recipient_owner,
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& recipient_existing_claims,
    const pathfinder::knowledge::KnowledgePropagationChannel& channel,
    const pathfinder::knowledge::KnowledgePropagationTrustBand& trust_band,
    double trust_score,
    pathfinder::foundation::Tick tick,
    const std::string& loop_key) const {

    BridgeResult result;

    // Build propagation attempt for exactly the specified source claim.
    pathfinder::knowledge::KnowledgePropagationAttempt attempt;
    attempt.attempt_key = loop_key;
    attempt.propagator.owner = source_claim.owner;
    attempt.propagator.available_knowledge_ids.push_back(source_claim.knowledge_id);

    pathfinder::knowledge::KnowledgePropagationSourceClaim sc;
    sc.claim = source_claim;
    sc.selected = true;
    sc.explicit_teaching_intent = true;
    attempt.source_claims.push_back(sc);

    attempt.target.owner = recipient_owner;
    for (const auto& c : recipient_existing_claims) {
        attempt.target.known_knowledge_ids.push_back(c.knowledge_id);
    }
    attempt.target_existing_claims = recipient_existing_claims;

    attempt.context.channel = channel;
    attempt.context.trust_band = trust_band;
    attempt.context.trust_score = trust_score;
    attempt.context.distance_factor = 1.0;
    attempt.context.channel_quality = 1.0;
    attempt.context.noise_factor = 0.0;
    attempt.context.propagation_tick = tick;
    attempt.context.is_correction = false;
    attempt.reason_keys.push_back("p50_teaching_loop_bridge");

    pathfinder::knowledge::KnowledgePropagationOptions options;
    options.min_source_confidence = 0.0;
    options.min_transfer_score = 0.0;
    options.allow_create_recipient_claim = true;
    options.allow_update_recipient_claim = true;
    options.allow_same_owner = false;
    options.allow_deprecated = false;
    options.allow_disproven = false;

    pathfinder::knowledge::KnowledgePropagationService propagation_service;
    auto prop_r = propagation_service.propagate(attempt, options);
    if (prop_r.is_error()) {
        result.ok = false;
        result.failure_kind = WorldTeachingFailureKind::PropagationFailed;
        result.reason_keys.push_back("propagation_service_failed");
        return result;
    }

    const auto& pres = prop_r.value();

    // Apply propagation to recipient snapshot
    pathfinder::knowledge::KnowledgePropagationApplier applier;
    auto apply_r = applier.applyToTargetSnapshot(recipient_existing_claims, pres);
    if (apply_r.is_error()) {
        result.ok = false;
        result.failure_kind = WorldTeachingFailureKind::PropagationFailed;
        result.reason_keys.push_back("propagation_applier_failed");
        return result;
    }

    result.recipient_snapshot_after = apply_r.value();
    result.recipient_created_claims = pres.created_claims;
    result.recipient_updated_claims = pres.updated_claims;

    // Ensure propagated claims inherit usable_for_action from source claim
    for (auto& claim : result.recipient_created_claims) {
        claim.projection_flags.usable_for_action = source_claim.projection_flags.usable_for_action;
        claim.projection_flags.usable_by_ai = source_claim.projection_flags.usable_by_ai;
    }
    for (auto& claim : result.recipient_updated_claims) {
        claim.projection_flags.usable_for_action = source_claim.projection_flags.usable_for_action;
        claim.projection_flags.usable_by_ai = source_claim.projection_flags.usable_by_ai;
    }
    for (auto& claim : result.recipient_snapshot_after) {
        for (const auto& created : result.recipient_created_claims) {
            if (claim.knowledge_id == created.knowledge_id) {
                claim.projection_flags.usable_for_action = source_claim.projection_flags.usable_for_action;
                claim.projection_flags.usable_by_ai = source_claim.projection_flags.usable_by_ai;
            }
        }
        for (const auto& updated : result.recipient_updated_claims) {
            if (claim.knowledge_id == updated.knowledge_id) {
                claim.projection_flags.usable_for_action = source_claim.projection_flags.usable_for_action;
                claim.projection_flags.usable_by_ai = source_claim.projection_flags.usable_by_ai;
            }
        }
    }

    // Map propagation decision to teaching decision
    switch (pres.decision) {
        case pathfinder::knowledge::KnowledgePropagationDecision::CreatedRecipientClaim:
            result.decision = WorldTeachingDecision::TaughtNewClaim;
            break;
        case pathfinder::knowledge::KnowledgePropagationDecision::ReinforcedRecipientClaim:
        case pathfinder::knowledge::KnowledgePropagationDecision::UpdatedRecipientClaim:
            result.decision = WorldTeachingDecision::ReinforcedExistingClaim;
            break;
        case pathfinder::knowledge::KnowledgePropagationDecision::CorrectionDelivered:
            result.decision = WorldTeachingDecision::RevisedExistingClaim;
            break;
        case pathfinder::knowledge::KnowledgePropagationDecision::Skipped:
            result.decision = WorldTeachingDecision::SkippedAlreadyKnown;
            break;
        case pathfinder::knowledge::KnowledgePropagationDecision::Rejected:
            result.decision = WorldTeachingDecision::Rejected;
            break;
        case pathfinder::knowledge::KnowledgePropagationDecision::Failed:
            result.decision = WorldTeachingDecision::Failed;
            result.failure_kind = WorldTeachingFailureKind::PropagationFailed;
            break;
        default:
            result.decision = WorldTeachingDecision::Failed;
            result.failure_kind = WorldTeachingFailureKind::PropagationFailed;
            break;
    }

    result.ok = (result.decision != WorldTeachingDecision::Failed &&
                 result.decision != WorldTeachingDecision::Rejected);
    if (result.ok) {
        result.reason_keys.push_back("teaching_propagation_succeeded");
    }
    return result;
}

} // namespace pathfinder::world_teaching
