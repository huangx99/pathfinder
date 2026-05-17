#pragma once

#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_revision.h"
#include <optional>
#include <vector>

namespace pathfinder::knowledge {

// ============================================================
// DTOs
// ============================================================

struct KnowledgePropagationOptions {
    double min_source_confidence = 0.35;
    double min_transfer_score = 0.25;
    double direct_teaching_weight = 0.85;
    double demonstration_weight = 0.70;
    double warning_weight = 0.75;
    double shared_observation_weight = 0.65;
    double tribe_instruction_weight = 0.80;
    double correction_weight = 0.90;
    double max_created_confidence = 0.80;
    double max_shared_confidence = 0.70;
    bool allow_agent_to_agent = true;
    bool allow_agent_to_tribe = true;
    bool allow_tribe_to_agent = true;
    bool allow_same_owner = false;
    bool allow_deprecated = false;
    bool allow_disproven = false;
    bool allow_correction = true;
    bool allow_create_recipient_claim = true;
    bool allow_update_recipient_claim = true;
    size_t max_claims_per_attempt = 8;
    size_t max_evidence_refs = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagator {
    KnowledgeOwner owner;
    std::vector<pathfinder::foundation::KnowledgeId> available_knowledge_ids;
    std::vector<std::string> capability_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationTarget {
    KnowledgeOwner owner;
    std::vector<pathfinder::foundation::KnowledgeId> known_knowledge_ids;
    std::vector<std::string> capability_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationContext {
    KnowledgePropagationChannel channel = KnowledgePropagationChannel::Unknown;
    KnowledgePropagationTrustBand trust_band = KnowledgePropagationTrustBand::Unknown;
    double trust_score = 0.5;
    double distance_factor = 1.0;
    double channel_quality = 1.0;
    double noise_factor = 0.0;
    bool receiver_can_ask = false;
    bool is_correction = false;
    pathfinder::foundation::Tick propagation_tick;
    std::vector<std::string> condition_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationSourceClaim {
    KnowledgeClaim claim;
    bool selected = true;
    bool explicit_teaching_intent = false;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationAttempt {
    std::string attempt_key;
    KnowledgePropagator propagator;
    KnowledgePropagationTarget target;
    KnowledgePropagationContext context;
    std::vector<KnowledgePropagationSourceClaim> source_claims;
    std::vector<KnowledgeClaim> target_existing_claims;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeTransferAssessment {
    pathfinder::foundation::KnowledgeId source_knowledge_id;
    KnowledgePropagationDecision recommended_decision = KnowledgePropagationDecision::Unknown;
    KnowledgePropagationFailureReason failure_reason = KnowledgePropagationFailureReason::Unknown;
    double transfer_score = 0.0;
    double created_confidence = 0.0;
    bool can_transfer = false;
    bool should_create_claim = false;
    bool should_update_existing = false;
    bool should_route_to_revision = false;
    std::vector<pathfinder::foundation::KnowledgeId> matched_target_claim_ids;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeRecipientClaimDraft {
    KnowledgeClaim claim;
    pathfinder::foundation::KnowledgeId source_knowledge_id;
    KnowledgeOwner source_owner;
    KnowledgePropagationChannel channel = KnowledgePropagationChannel::Unknown;
    double transfer_score = 0.0;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationPlan {
    std::string plan_key;
    KnowledgePropagationAttempt attempt;
    std::vector<KnowledgeTransferAssessment> assessments;
    std::vector<KnowledgeRecipientClaimDraft> recipient_claim_drafts;
    std::vector<KnowledgeClaimPatch> target_revision_patches;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePropagationResult {
    KnowledgePropagationDecision decision = KnowledgePropagationDecision::Unknown;
    std::optional<KnowledgePropagationPlan> plan;
    std::vector<KnowledgeClaim> created_claims;
    std::vector<KnowledgeClaim> updated_claims;
    std::vector<KnowledgeClaimPatch> applied_patches;
    std::vector<KnowledgeEventDraft> event_drafts;
    std::vector<KnowledgeStateChangeDraft> state_changes;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Services
// ============================================================

class KnowledgePropagationPlanner {
public:
    pathfinder::foundation::Result<KnowledgePropagationPlan> planPropagation(
        const KnowledgePropagationAttempt& attempt,
        const KnowledgePropagationOptions& options = {}) const;
};

class KnowledgePropagationService {
public:
    pathfinder::foundation::Result<KnowledgePropagationResult> propagate(
        const KnowledgePropagationAttempt& attempt,
        const KnowledgePropagationOptions& options = {}) const;
};

class KnowledgePropagationApplier {
public:
    pathfinder::foundation::Result<std::vector<KnowledgeClaim>> applyToTargetSnapshot(
        const std::vector<KnowledgeClaim>& current_target_claims,
        const KnowledgePropagationResult& result) const;
};

} // namespace pathfinder::knowledge
