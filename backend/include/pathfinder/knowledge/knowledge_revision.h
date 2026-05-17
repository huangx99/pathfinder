#pragma once

#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_formation.h"
#include <optional>
#include <vector>

namespace pathfinder::knowledge {

// ============================================================
// DTOs
// ============================================================

struct KnowledgeRevisionOptions {
    double reinforce_weight = 0.12;
    double weak_conflict_penalty = 0.15;
    double strong_conflict_penalty = 0.35;
    double disprove_confidence_max = 0.20;
    size_t strong_conflict_min_count = 2;
    bool prefer_specialization = true;
    bool allow_deprecated = true;
    bool allow_disproven = true;
    bool allow_create_specialized_claim = true;
    bool keep_old_claim_history = true;
    size_t max_revision_evidence_refs = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeConflictSignal {
    KnowledgeConflictType conflict_type = KnowledgeConflictType::Unknown;
    pathfinder::foundation::KnowledgeId existing_knowledge_id;
    KnowledgeSubject subject;
    KnowledgePredicate existing_predicate;
    KnowledgePredicate incoming_predicate;
    std::vector<KnowledgeCondition> existing_conditions;
    std::vector<KnowledgeCondition> incoming_conditions;
    std::vector<KnowledgeEvidence> evidence_refs;
    double severity = 0.0;
    bool condition_explains_conflict = false;
    bool should_specialize = false;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeRevisionInput {
    KnowledgeFormationInput formation_input;
    std::vector<KnowledgeClaim> existing_claims;
    pathfinder::foundation::Tick revision_tick;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeRevisionAssessment {
    std::vector<pathfinder::foundation::KnowledgeId> matched_claim_ids;
    std::vector<KnowledgeConflictSignal> conflict_signals;
    KnowledgeResolutionStrategy recommended_strategy = KnowledgeResolutionStrategy::Unknown;
    double confidence_delta = 0.0;
    bool has_support = false;
    bool has_conflict = false;
    bool should_create_specialized_claim = false;
    bool should_deprecate_existing = false;
    bool should_disprove_existing = false;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeClaimPatch {
    pathfinder::foundation::KnowledgeId knowledge_id;
    KnowledgeStatus status_before = KnowledgeStatus::Unknown;
    KnowledgeStatus status_after = KnowledgeStatus::Unknown;
    KnowledgeConfidence confidence_before;
    KnowledgeConfidence confidence_after;
    std::vector<KnowledgeEvidence> added_evidence_refs;
    pathfinder::foundation::KnowledgeId superseded_by_knowledge_id;
    std::vector<pathfinder::foundation::KnowledgeId> supersedes_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeRevisionPlan {
    std::string plan_key;
    KnowledgeRevisionInput input;
    KnowledgeRevisionAssessment assessment;
    std::vector<KnowledgeClaimPatch> patches;
    std::vector<KnowledgeClaim> candidate_new_claims;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeRevisionResult {
    KnowledgeRevisionDecision decision = KnowledgeRevisionDecision::Unknown;
    std::optional<KnowledgeRevisionPlan> plan;
    std::vector<KnowledgeClaim> updated_claims;
    std::vector<KnowledgeClaim> created_claims;
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

class KnowledgeRevisionPlanner {
public:
    pathfinder::foundation::Result<KnowledgeRevisionPlan> planRevision(
        const KnowledgeRevisionInput& input,
        const KnowledgeRevisionOptions& options = {}) const;
};

class KnowledgeRevisionService {
public:
    pathfinder::foundation::Result<KnowledgeRevisionResult> revise(
        const KnowledgeRevisionInput& input,
        const KnowledgeRevisionOptions& options = {}) const;
};

class KnowledgeRevisionApplier {
public:
    pathfinder::foundation::Result<std::vector<KnowledgeClaim>> applyToSnapshot(
        const std::vector<KnowledgeClaim>& current_claims,
        const KnowledgeRevisionResult& result) const;
};

} // namespace pathfinder::knowledge
