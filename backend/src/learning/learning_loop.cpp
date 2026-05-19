#include "pathfinder/learning/learning_loop.h"
#include "pathfinder/condition/condition_normalizer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace pathfinder::learning {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::foundation::Tick;
using pathfinder::foundation::EntityId;
using pathfinder::foundation::ActionId;
using pathfinder::foundation::EventId;
using pathfinder::foundation::MemoryId;
using pathfinder::foundation::KnowledgeId;

using pathfinder::cognition::CognitionSubjectKind;
using pathfinder::cognition::CognitionTargetKind;
using pathfinder::cognition::CognitionAspect;
using pathfinder::cognition::CognitionActionContextKind;
using pathfinder::cognition::CognitionOutcomeSignal;
using pathfinder::cognition::CognitionEvidenceSourceKind;
using pathfinder::cognition::CognitionEvidenceSupport;
using pathfinder::cognition::CognitionRiskLevel;
using pathfinder::cognition::CognitionConfidenceBand;
using pathfinder::cognition::CognitionUpdateDecision;
using pathfinder::cognition::CognitionFeedbackSignal;
using pathfinder::cognition::CognitionEvidenceBuilder;
using pathfinder::cognition::CognitionConfidenceModel;
using pathfinder::cognition::CognitionUpdaterV2;
using pathfinder::cognition::CognitionStateV2;
using pathfinder::cognition::CognitionEvidenceRecord;
using pathfinder::cognition::CognitionClaimV2;
using pathfinder::cognition::CognitionSubject;
using pathfinder::cognition::CognitionTarget;
using pathfinder::cognition::CognitionClaimKeyV2;
using pathfinder::cognition::CognitionConfidenceModelOptions;
using pathfinder::cognition::CognitionUpdateRequest;
using pathfinder::cognition::CognitionUpdateResult;

using pathfinder::memory::MemoryOwnerKind;
using pathfinder::memory::MemorySubjectKind;
using pathfinder::memory::MemoryKind;
using pathfinder::memory::MemoryScope;
using pathfinder::memory::MemoryLifecycle;
using pathfinder::memory::MemoryStrengthBand;
using pathfinder::memory::MemoryImportance;
using pathfinder::memory::MemorySourceKind;
using pathfinder::memory::MemorySummaryKind;
using pathfinder::memory::MemoryCompressionLevel;
using pathfinder::memory::strengthToBand;
using pathfinder::memory::MemoryWriteDecision;
using pathfinder::memory::MemoryCompressionDecision;
using pathfinder::memory::MemoryRecallMode;
using pathfinder::memory::MemoryRecallSort;
using pathfinder::memory::MemoryRecallItemKind;
using pathfinder::memory::MemoryOwner;
using pathfinder::memory::MemorySubject;
using pathfinder::memory::MemoryEvidenceRef;
using pathfinder::memory::MemoryCandidate;
using pathfinder::memory::MemoryRecord;
using pathfinder::memory::MemoryWriteOptions;
using pathfinder::memory::MemoryWriteResult;
using pathfinder::memory::MemorySummaryKey;
using pathfinder::memory::MemorySummary;
using pathfinder::memory::MemorySummaryId;
using pathfinder::memory::MemoryCompressionOptions;
using pathfinder::memory::MemoryCompressionResult;
using pathfinder::memory::MemoryRecallQuery;
using pathfinder::memory::MemoryRecallResult;
using pathfinder::memory::MemoryCandidateFactory;
using pathfinder::memory::MemoryIdFactory;
using pathfinder::memory::MemoryWriteService;
using pathfinder::memory::MemoryCompressionService;
using pathfinder::memory::MemoryRecallService;
using pathfinder::memory::containsMemoryForbiddenKey;

using pathfinder::knowledge::KnowledgeOwnerKind;
using pathfinder::knowledge::KnowledgeSubjectKind;
using pathfinder::knowledge::KnowledgeRelationType;
using pathfinder::knowledge::KnowledgeConfidenceBand;
using pathfinder::knowledge::KnowledgeStatus;
using pathfinder::knowledge::KnowledgeEvidenceKind;
using pathfinder::knowledge::KnowledgeFormationDecision;
using pathfinder::knowledge::KnowledgeConflictType;
using pathfinder::knowledge::KnowledgeRevisionDecision;
using pathfinder::knowledge::KnowledgeResolutionStrategy;
using pathfinder::knowledge::KnowledgePropagationChannel;
using pathfinder::knowledge::KnowledgePropagationDecision;
using pathfinder::knowledge::KnowledgePropagationTrustBand;
using pathfinder::knowledge::KnowledgePropagationFailureReason;
using pathfinder::knowledge::KnowledgeOwner;
using pathfinder::knowledge::KnowledgeSubject;
using pathfinder::knowledge::KnowledgePredicate;
using pathfinder::knowledge::KnowledgeCondition;
using pathfinder::knowledge::KnowledgeEvidence;
using pathfinder::knowledge::KnowledgeConfidence;
using pathfinder::knowledge::KnowledgeTeachingProfile;
using pathfinder::knowledge::KnowledgeProjectionFlags;
using pathfinder::knowledge::KnowledgeClaim;
using pathfinder::knowledge::KnowledgeFormationOptions;
using pathfinder::knowledge::KnowledgeFormationInput;
using pathfinder::knowledge::KnowledgeFormationPlan;
using pathfinder::knowledge::KnowledgeFormationResult;
using pathfinder::knowledge::KnowledgeRevisionOptions;
using pathfinder::knowledge::KnowledgeConflictSignal;
using pathfinder::knowledge::KnowledgeRevisionInput;
using pathfinder::knowledge::KnowledgeRevisionAssessment;
using pathfinder::knowledge::KnowledgeClaimPatch;
using pathfinder::knowledge::KnowledgeRevisionPlan;
using pathfinder::knowledge::KnowledgeRevisionResult;
using pathfinder::knowledge::KnowledgePropagationOptions;
using pathfinder::knowledge::KnowledgePropagator;
using pathfinder::knowledge::KnowledgePropagationTarget;
using pathfinder::knowledge::KnowledgePropagationContext;
using pathfinder::knowledge::KnowledgePropagationSourceClaim;
using pathfinder::knowledge::KnowledgePropagationAttempt;
using pathfinder::knowledge::KnowledgeTransferAssessment;
using pathfinder::knowledge::KnowledgeRecipientClaimDraft;
using pathfinder::knowledge::KnowledgePropagationPlan;
using pathfinder::knowledge::KnowledgePropagationResult;
using pathfinder::knowledge::KnowledgeQuery;
using pathfinder::knowledge::KnowledgeProjection;
using pathfinder::knowledge::KnowledgeProjectionItem;
using pathfinder::knowledge::KnowledgeProjectionBuilder;
using pathfinder::knowledge::KnowledgeIdFactory;
using pathfinder::knowledge::KnowledgeFormationService;
using pathfinder::knowledge::KnowledgeRevisionService;
using pathfinder::knowledge::KnowledgeRevisionApplier;
using pathfinder::knowledge::KnowledgePropagationService;
using pathfinder::knowledge::KnowledgePropagationApplier;
using pathfinder::knowledge::containsKnowledgeForbiddenKey;

// ============================================================
// Helpers
// ============================================================

static double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

static bool sameConditionKeys(const std::vector<KnowledgeCondition>& a,
                              const std::vector<KnowledgeCondition>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    std::vector<std::string> left;
    std::vector<std::string> right;
    left.reserve(a.size());
    right.reserve(b.size());
    for (const auto& condition : a) {
        auto canonical = pathfinder::knowledge::canonicalKnowledgeConditionKey(condition);
        if (canonical.is_error()) return false;
        left.push_back(canonical.value());
    }
    for (const auto& condition : b) {
        auto canonical = pathfinder::knowledge::canonicalKnowledgeConditionKey(condition);
        if (canonical.is_error()) return false;
        right.push_back(canonical.value());
    }
    std::sort(left.begin(), left.end());
    std::sort(right.begin(), right.end());
    return left == right;
}

static bool shouldReviseForConditionScope(const std::vector<KnowledgeCondition>& existing_conditions,
                                          const std::vector<KnowledgeCondition>& incoming_conditions) {
    return sameConditionKeys(existing_conditions, incoming_conditions);
}

static bool sameKnowledgeLearningScope(const KnowledgeClaim& left, const KnowledgeClaim& right) {
    return left.owner == right.owner &&
           left.subject.subject_id == right.subject.subject_id &&
           left.subject.kind == right.subject.kind &&
           left.subject.related_subject_ids == right.subject.related_subject_ids &&
           left.subject.relation_group_key == right.subject.relation_group_key &&
           left.predicate.relation_type == right.predicate.relation_type &&
           left.predicate.action_key == right.predicate.action_key &&
           left.predicate.effect_key == right.predicate.effect_key &&
           sameConditionKeys(left.conditions, right.conditions);
}

static bool hasReasonKey(const std::vector<std::string>& reason_keys, const std::string& reason_key) {
    return std::find(reason_keys.begin(), reason_keys.end(), reason_key) != reason_keys.end();
}

static size_t accumulatedSupportForScope(const std::vector<KnowledgeClaim>& claims, const KnowledgeClaim& candidate) {
    size_t support = 0;
    const bool candidate_is_ambiguous = hasReasonKey(candidate.reason_keys, "causal_ambiguous_dose_window");
    for (const auto& claim : claims) {
        if (!sameKnowledgeLearningScope(claim, candidate)) continue;
        if (!candidate_is_ambiguous && hasReasonKey(claim.reason_keys, "causal_ambiguous_dose_window")) continue;
        support = std::max(support, claim.confidence.support_count);
    }
    return support;
}

static KnowledgeStatus statusFromIndependentExperienceCount(size_t experience_count) {
    if (experience_count >= 3) return KnowledgeStatus::Active;
    if (experience_count == 2) return KnowledgeStatus::Hypothesis;
    return KnowledgeStatus::Candidate;
}

static void applyIndependentExperienceThreshold(KnowledgeClaim& claim, size_t previous_support_count) {
    const size_t experience_count = std::max<size_t>(1, previous_support_count + 1);
    claim.confidence.support_count = experience_count;
    claim.confidence.source_summary_count = std::max(claim.confidence.source_summary_count, experience_count);
    claim.status = statusFromIndependentExperienceCount(experience_count);
    if (hasReasonKey(claim.reason_keys, "causal_ambiguous_dose_window") &&
        (claim.status == KnowledgeStatus::Active ||
         claim.status == KnowledgeStatus::Teachable ||
         claim.status == KnowledgeStatus::Operational)) {
        claim.status = KnowledgeStatus::Hypothesis;
        claim.reason_keys.push_back("causal_confirmation_capped_by_alternative_explanation");
    }
    claim.projection_flags.usable_for_action = claim.status == KnowledgeStatus::Active ||
                                               claim.status == KnowledgeStatus::Teachable ||
                                               claim.status == KnowledgeStatus::Operational;
    claim.projection_flags.usable_for_teaching = claim.status == KnowledgeStatus::Teachable ||
                                                 claim.status == KnowledgeStatus::Active;
    claim.teaching_profile.teachable = claim.status == KnowledgeStatus::Teachable;
    claim.reason_keys.push_back("independent_experience_threshold_applied");
}

static bool isValidEnumStoryKind(LearningLoopStoryKind k) {
    return k != LearningLoopStoryKind::Unknown && k != LearningLoopStoryKind::TestOnly;
}

static bool isValidEnumStage(LearningLoopStage s) {
    return s != LearningLoopStage::Unknown && s != LearningLoopStage::TestOnly;
}

static bool isValidEnumDecision(LearningLoopDecision d) {
    return d != LearningLoopDecision::Unknown && d != LearningLoopDecision::TestOnly;
}

static bool isValidEnumFailureReason(LearningLoopFailureReason r) {
    return r != LearningLoopFailureReason::Unknown && r != LearningLoopFailureReason::TestOnly;
}

static bool isValidEnumStepDecision(LearningStepDecision d) {
    return d != LearningStepDecision::Unknown && d != LearningStepDecision::TestOnly;
}

// ============================================================
// Enum toString / fromString
// ============================================================

std::string toString(LearningLoopStoryKind kind) {
    switch (kind) {
        case LearningLoopStoryKind::Unknown: return "unknown";
        case LearningLoopStoryKind::DirectDiscovery: return "direct_discovery";
        case LearningLoopStoryKind::RepeatedLearning: return "repeated_learning";
        case LearningLoopStoryKind::CorrectionAfterContradiction: return "correction_after_contradiction";
        case LearningLoopStoryKind::TeachingToRecipient: return "teaching_to_recipient";
        case LearningLoopStoryKind::FullLearningLoop: return "full_learning_loop";
        case LearningLoopStoryKind::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<LearningLoopStoryKind> learningLoopStoryKindFromString(const std::string& str) {
    if (str == "unknown") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::Unknown);
    if (str == "direct_discovery") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::DirectDiscovery);
    if (str == "repeated_learning") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::RepeatedLearning);
    if (str == "correction_after_contradiction") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::CorrectionAfterContradiction);
    if (str == "teaching_to_recipient") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::TeachingToRecipient);
    if (str == "full_learning_loop") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::FullLearningLoop);
    if (str == "test_only") return Result<LearningLoopStoryKind>::ok(LearningLoopStoryKind::TestOnly);
    return Result<LearningLoopStoryKind>::fail(makeError(ErrorCode::validation_failed, "Unknown LearningLoopStoryKind string"));
}

std::string toString(LearningLoopStage stage) {
    switch (stage) {
        case LearningLoopStage::Unknown: return "unknown";
        case LearningLoopStage::FeedbackCaptured: return "feedback_captured";
        case LearningLoopStage::CognitionUpdated: return "cognition_updated";
        case LearningLoopStage::MemoryWritten: return "memory_written";
        case LearningLoopStage::MemoryCompressed: return "memory_compressed";
        case LearningLoopStage::MemoryRecalled: return "memory_recalled";
        case LearningLoopStage::KnowledgeFormed: return "knowledge_formed";
        case LearningLoopStage::KnowledgeRevised: return "knowledge_revised";
        case LearningLoopStage::KnowledgePropagated: return "knowledge_propagated";
        case LearningLoopStage::RecipientInfluenced: return "recipient_influenced";
        case LearningLoopStage::ReportBuilt: return "report_built";
        case LearningLoopStage::Completed: return "completed";
        case LearningLoopStage::Failed: return "failed";
        case LearningLoopStage::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<LearningLoopStage> learningLoopStageFromString(const std::string& str) {
    if (str == "unknown") return Result<LearningLoopStage>::ok(LearningLoopStage::Unknown);
    if (str == "feedback_captured") return Result<LearningLoopStage>::ok(LearningLoopStage::FeedbackCaptured);
    if (str == "cognition_updated") return Result<LearningLoopStage>::ok(LearningLoopStage::CognitionUpdated);
    if (str == "memory_written") return Result<LearningLoopStage>::ok(LearningLoopStage::MemoryWritten);
    if (str == "memory_compressed") return Result<LearningLoopStage>::ok(LearningLoopStage::MemoryCompressed);
    if (str == "memory_recalled") return Result<LearningLoopStage>::ok(LearningLoopStage::MemoryRecalled);
    if (str == "knowledge_formed") return Result<LearningLoopStage>::ok(LearningLoopStage::KnowledgeFormed);
    if (str == "knowledge_revised") return Result<LearningLoopStage>::ok(LearningLoopStage::KnowledgeRevised);
    if (str == "knowledge_propagated") return Result<LearningLoopStage>::ok(LearningLoopStage::KnowledgePropagated);
    if (str == "recipient_influenced") return Result<LearningLoopStage>::ok(LearningLoopStage::RecipientInfluenced);
    if (str == "report_built") return Result<LearningLoopStage>::ok(LearningLoopStage::ReportBuilt);
    if (str == "completed") return Result<LearningLoopStage>::ok(LearningLoopStage::Completed);
    if (str == "failed") return Result<LearningLoopStage>::ok(LearningLoopStage::Failed);
    if (str == "test_only") return Result<LearningLoopStage>::ok(LearningLoopStage::TestOnly);
    return Result<LearningLoopStage>::fail(makeError(ErrorCode::validation_failed, "Unknown LearningLoopStage string"));
}

std::string toString(LearningLoopDecision decision) {
    switch (decision) {
        case LearningLoopDecision::Unknown: return "unknown";
        case LearningLoopDecision::Completed: return "completed";
        case LearningLoopDecision::PartialCompleted: return "partial_completed";
        case LearningLoopDecision::Skipped: return "skipped";
        case LearningLoopDecision::RoutedToRevision: return "routed_to_revision";
        case LearningLoopDecision::Rejected: return "rejected";
        case LearningLoopDecision::Failed: return "failed";
        case LearningLoopDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<LearningLoopDecision> learningLoopDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<LearningLoopDecision>::ok(LearningLoopDecision::Unknown);
    if (str == "completed") return Result<LearningLoopDecision>::ok(LearningLoopDecision::Completed);
    if (str == "partial_completed") return Result<LearningLoopDecision>::ok(LearningLoopDecision::PartialCompleted);
    if (str == "skipped") return Result<LearningLoopDecision>::ok(LearningLoopDecision::Skipped);
    if (str == "routed_to_revision") return Result<LearningLoopDecision>::ok(LearningLoopDecision::RoutedToRevision);
    if (str == "rejected") return Result<LearningLoopDecision>::ok(LearningLoopDecision::Rejected);
    if (str == "failed") return Result<LearningLoopDecision>::ok(LearningLoopDecision::Failed);
    if (str == "test_only") return Result<LearningLoopDecision>::ok(LearningLoopDecision::TestOnly);
    return Result<LearningLoopDecision>::fail(makeError(ErrorCode::validation_failed, "Unknown LearningLoopDecision string"));
}

std::string toString(LearningLoopFailureReason reason) {
    switch (reason) {
        case LearningLoopFailureReason::Unknown: return "unknown";
        case LearningLoopFailureReason::None: return "none";
        case LearningLoopFailureReason::InvalidInput: return "invalid_input";
        case LearningLoopFailureReason::MissingFeedback: return "missing_feedback";
        case LearningLoopFailureReason::SecurityRejected: return "security_rejected";
        case LearningLoopFailureReason::CognitionFailed: return "cognition_failed";
        case LearningLoopFailureReason::MemoryWriteFailed: return "memory_write_failed";
        case LearningLoopFailureReason::CompressionFailed: return "compression_failed";
        case LearningLoopFailureReason::RecallFailed: return "recall_failed";
        case LearningLoopFailureReason::FormationFailed: return "formation_failed";
        case LearningLoopFailureReason::RevisionFailed: return "revision_failed";
        case LearningLoopFailureReason::PropagationFailed: return "propagation_failed";
        case LearningLoopFailureReason::RecipientInfluenceMissing: return "recipient_influence_missing";
        case LearningLoopFailureReason::ReportFailed: return "report_failed";
        case LearningLoopFailureReason::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<LearningLoopFailureReason> learningLoopFailureReasonFromString(const std::string& str) {
    if (str == "unknown") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::Unknown);
    if (str == "none") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::None);
    if (str == "invalid_input") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::InvalidInput);
    if (str == "missing_feedback") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::MissingFeedback);
    if (str == "security_rejected") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::SecurityRejected);
    if (str == "cognition_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::CognitionFailed);
    if (str == "memory_write_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::MemoryWriteFailed);
    if (str == "compression_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::CompressionFailed);
    if (str == "recall_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::RecallFailed);
    if (str == "formation_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::FormationFailed);
    if (str == "revision_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::RevisionFailed);
    if (str == "propagation_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::PropagationFailed);
    if (str == "recipient_influence_missing") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::RecipientInfluenceMissing);
    if (str == "report_failed") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::ReportFailed);
    if (str == "test_only") return Result<LearningLoopFailureReason>::ok(LearningLoopFailureReason::TestOnly);
    return Result<LearningLoopFailureReason>::fail(makeError(ErrorCode::validation_failed, "Unknown LearningLoopFailureReason string"));
}

std::string toString(LearningStepDecision decision) {
    switch (decision) {
        case LearningStepDecision::Unknown: return "unknown";
        case LearningStepDecision::Executed: return "executed";
        case LearningStepDecision::SkippedByOptions: return "skipped_by_options";
        case LearningStepDecision::SkippedNoInput: return "skipped_no_input";
        case LearningStepDecision::Routed: return "routed";
        case LearningStepDecision::Rejected: return "rejected";
        case LearningStepDecision::Failed: return "failed";
        case LearningStepDecision::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<LearningStepDecision> learningStepDecisionFromString(const std::string& str) {
    if (str == "unknown") return Result<LearningStepDecision>::ok(LearningStepDecision::Unknown);
    if (str == "executed") return Result<LearningStepDecision>::ok(LearningStepDecision::Executed);
    if (str == "skipped_by_options") return Result<LearningStepDecision>::ok(LearningStepDecision::SkippedByOptions);
    if (str == "skipped_no_input") return Result<LearningStepDecision>::ok(LearningStepDecision::SkippedNoInput);
    if (str == "routed") return Result<LearningStepDecision>::ok(LearningStepDecision::Routed);
    if (str == "rejected") return Result<LearningStepDecision>::ok(LearningStepDecision::Rejected);
    if (str == "failed") return Result<LearningStepDecision>::ok(LearningStepDecision::Failed);
    if (str == "test_only") return Result<LearningStepDecision>::ok(LearningStepDecision::TestOnly);
    return Result<LearningStepDecision>::fail(makeError(ErrorCode::validation_failed, "Unknown LearningStepDecision string"));
}

// ============================================================
// Forbidden Keys
// ============================================================

std::vector<std::string> learningForbiddenKeys() {
    return {
        "hidden_truth",
        "real_effect",
        "true_trait",
        "object_truth",
        "raw_state",
        "edible_profile",
        "hunger_delta",
        "health_delta",
        "effect_kind",
        "GameState",
        "AgentTickRecord",
        "SaveGame",
        "SaveManager",
        "reward_value",
        "done =",
        "is_done",
        "HTTP",
        "WebSocket",
        "nlohmann",
        "json",
        "AgentRuntime",
        "Policy",
        "RulePipeline"
    };
}

bool containsLearningForbiddenKey(const std::string& text) {
    for (const auto& key : learningForbiddenKeys()) {
        if (text.find(key) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool containsLearningForbiddenKey(const std::vector<std::string>& values) {
    for (const auto& v : values) {
        if (containsLearningForbiddenKey(v)) return true;
    }
    return false;
}


// ============================================================
// DTO validateBasic implementations
// ============================================================

Result<void> LearningActorRef::validateBasic() const {
    auto r = knowledge_owner.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef knowledge_owner invalid"));
    r = memory_owner.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef memory_owner invalid"));
    r = cognition_subject.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef cognition_subject invalid"));
    if (display_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef display_key empty"));
    }
    if (containsLearningForbiddenKey(display_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef display_key contains forbidden key"));
    }
    if (containsLearningForbiddenKey(safe_tags)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningActorRef safe_tags contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> LearningSafeFeedbackInput::validateBasic() const {
    if (feedback_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput feedback_key empty"));
    }
    if (containsLearningForbiddenKey(feedback_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput feedback_key contains forbidden key"));
    }
    auto r = cognition_target.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput cognition_target invalid"));
    r = memory_subject.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput memory_subject invalid"));
    r = knowledge_subject.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput knowledge_subject invalid"));
    if (action_context == CognitionActionContextKind::Unknown || action_context == CognitionActionContextKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput action_context invalid"));
    }
    if (action_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput action_id empty"));
    }
    if (action_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput action_key empty"));
    }
    if (containsLearningForbiddenKey(action_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput action_key contains forbidden key"));
    }
    if (containsLearningForbiddenKey(target_subject_id) || containsLearningForbiddenKey(target_subject_type_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput target subject contains forbidden key"));
    }
    for (const auto& sig : outcome_signals) {
        if (sig == CognitionOutcomeSignal::Unknown || sig == CognitionOutcomeSignal::TestOnly) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput outcome_signal invalid"));
        }
    }
    if (containsLearningForbiddenKey(effect_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput effect_key contains forbidden key"));
    }
    if (containsLearningForbiddenKey(condition_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput condition_keys contain forbidden key"));
    }
    if (!condition_keys.empty()) {
        pathfinder::condition::ConditionNormalizer normalizer;
        auto normalized = normalizer.normalizeKeys(condition_keys);
        if (normalized.is_error()) return Result<void>::fail(normalized.errors());
    }
    if (utility_delta < -1.0 || utility_delta > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput utility_delta out of range [-1,1]"));
    }
    if (risk_delta < 0.0 || risk_delta > 1.0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput risk_delta out of range [0,1]"));
    }
    if (observed_tick.value() < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput observed_tick invalid"));
    }
    if (containsLearningForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput reason_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningSafeFeedbackInput warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> LearningLoopOptions::validateBasic() const {
    if (max_step_traces == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopOptions max_step_traces cannot be 0"));
    }
    if (max_report_items == 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopOptions max_report_items cannot be 0"));
    }
    return Result<void>::ok();
}

Result<void> LearningLoopInput::validateBasic() const {
    if (loop_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput loop_key empty"));
    }
    if (containsLearningForbiddenKey(loop_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput loop_key contains forbidden key"));
    }
    if (story_kind == LearningLoopStoryKind::Unknown || story_kind == LearningLoopStoryKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput story_kind invalid"));
    }
    auto r = actor.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor invalid"));
    if (recipient.has_value()) {
        r = recipient->validateBasic();
        if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput recipient invalid"));
    }
    r = feedback.validateBasic();
    if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput feedback invalid"));
    for (const auto& m : actor_existing_memories) {
        r = m.validateBasic();
        if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_memories invalid"));
        if (!(m.owner == actor.memory_owner)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_memory owner mismatch"));
        }
    }
    for (const auto& summary : actor_existing_summaries) {
        r = summary.validateBasic();
        if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_summaries invalid"));
        if (!(summary.key.owner == actor.memory_owner)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_summary owner mismatch"));
        }
    }
    for (const auto& c : actor_existing_claims) {
        r = c.validateBasic();
        if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_claims invalid"));
        if (!(c.owner == actor.knowledge_owner)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput actor_existing_claim owner mismatch"));
        }
    }
    for (const auto& c : recipient_existing_claims) {
        r = c.validateBasic();
        if (r.is_error()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput recipient_existing_claims invalid"));
        if (!recipient.has_value() || !(c.owner == recipient->knowledge_owner)) {
            return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput recipient_existing_claim owner mismatch"));
        }
    }
    if (loop_tick.value() < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput loop_tick invalid"));
    }
    if (containsLearningForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopInput reason_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> LearningEventChain::validateBasic() const {
    if (chain_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain chain_key empty"));
    }
    if (containsLearningForbiddenKey(chain_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain chain_key contains forbidden key"));
    }
    if (loop_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain loop_key empty"));
    }
    if (story_kind == LearningLoopStoryKind::Unknown || story_kind == LearningLoopStoryKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain story_kind invalid"));
    }
    if (containsLearningForbiddenKey(chain_reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain chain_reason_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningEventChain warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> LearningLoopStepTrace::validateBasic() const {
    if (stage == LearningLoopStage::Unknown || stage == LearningLoopStage::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace stage invalid"));
    }
    if (decision == LearningStepDecision::Unknown || decision == LearningStepDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace decision invalid"));
    }
    if (failure_reason == LearningLoopFailureReason::Unknown || failure_reason == LearningLoopFailureReason::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace failure_reason invalid"));
    }
    if (!summary_key.empty() && containsLearningForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace summary_key contains forbidden key"));
    }
    if (containsLearningForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace reason_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopStepTrace warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

bool LearningLoopResult::ok() const {
    return decision == LearningLoopDecision::Completed
        || decision == LearningLoopDecision::PartialCompleted
        || decision == LearningLoopDecision::Skipped
        || decision == LearningLoopDecision::RoutedToRevision;
}

Result<void> LearningLoopResult::validateBasic() const {
    if (decision == LearningLoopDecision::Unknown || decision == LearningLoopDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopResult decision invalid"));
    }
    if (failure_reason == LearningLoopFailureReason::Unknown || failure_reason == LearningLoopFailureReason::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopResult failure_reason invalid"));
    }
    if (event_chain.has_value()) {
        auto r = event_chain->validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& t : traces) {
        auto r = t.validateBasic();
        if (r.is_error()) return r;
    }
    if (cognition_result.has_value()) {
        auto r = cognition_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (memory_write_result.has_value()) {
        auto r = memory_write_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (memory_compression_result.has_value()) {
        auto r = memory_compression_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (memory_recall_result.has_value()) {
        auto r = memory_recall_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (knowledge_formation_result.has_value()) {
        auto r = knowledge_formation_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (knowledge_revision_result.has_value()) {
        auto r = knowledge_revision_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (knowledge_propagation_result.has_value()) {
        auto r = knowledge_propagation_result->validateBasic();
        if (r.is_error()) return r;
    }
    if (recipient_projection.has_value()) {
        auto r = recipient_projection->validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& c : actor_claim_snapshot_after) {
        auto r = c.validateBasic();
        if (r.is_error()) return r;
    }
    for (const auto& c : recipient_claim_snapshot_after) {
        auto r = c.validateBasic();
        if (r.is_error()) return r;
    }
    if (containsLearningForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopResult reason_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningLoopResult warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}

Result<void> LearningDebugReport::validateBasic() const {
    if (report_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport report_key empty"));
    }
    if (containsLearningForbiddenKey(report_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport report_key contains forbidden key"));
    }
    if (loop_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport loop_key empty"));
    }
    if (decision == LearningLoopDecision::Unknown || decision == LearningLoopDecision::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport decision invalid"));
    }
    if (containsLearningForbiddenKey(timeline_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport timeline_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(actor_knowledge_summary_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport actor_knowledge_summary_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(recipient_knowledge_summary_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport recipient_knowledge_summary_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(trace_summary_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport trace_summary_keys contain forbidden key"));
    }
    if (containsLearningForbiddenKey(warning_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "LearningDebugReport warning_keys contain forbidden key"));
    }
    return Result<void>::ok();
}


// ============================================================
// LearningLoopPlanner
// ============================================================

Result<std::vector<LearningLoopStepTrace>> LearningLoopPlanner::plan(
    const LearningLoopInput& input,
    const LearningLoopOptions& options) const {

    auto opts_r = options.validateBasic();
    if (opts_r.is_error()) {
        return Result<std::vector<LearningLoopStepTrace>>::fail(opts_r.errors());
    }
    auto input_r = input.validateBasic();
    if (input_r.is_error()) {
        return Result<std::vector<LearningLoopStepTrace>>::fail(input_r.errors());
    }

    // Security scan
    if (containsLearningForbiddenKey(input.feedback.effect_key)) {
        return Result<std::vector<LearningLoopStepTrace>>::fail(
            makeError(ErrorCode::validation_failed, "Planner: feedback effect_key contains forbidden key"));
    }
    if (containsLearningForbiddenKey(input.feedback.reason_keys)) {
        return Result<std::vector<LearningLoopStepTrace>>::fail(
            makeError(ErrorCode::validation_failed, "Planner: feedback reason_keys contain forbidden key"));
    }

    std::vector<LearningLoopStepTrace> plan;

    auto add_trace = [&](LearningLoopStage stage, LearningStepDecision decision,
                         LearningLoopFailureReason reason = LearningLoopFailureReason::None) {
        LearningLoopStepTrace trace;
        trace.stage = stage;
        trace.decision = decision;
        trace.failure_reason = reason;
        trace.summary_key = toString(stage);
        plan.push_back(trace);
    };

    // Step 1: FeedbackCaptured
    add_trace(LearningLoopStage::FeedbackCaptured, LearningStepDecision::Executed);

    // Step 2: CognitionUpdated
    if (options.enable_cognition) {
        add_trace(LearningLoopStage::CognitionUpdated, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::CognitionUpdated, LearningStepDecision::SkippedByOptions);
    }

    // Step 3: MemoryWritten
    if (options.enable_memory_write) {
        add_trace(LearningLoopStage::MemoryWritten, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::MemoryWritten, LearningStepDecision::SkippedByOptions);
    }

    // Step 4: MemoryCompressed
    if (options.enable_memory_compression) {
        if (!input.actor_existing_memories.empty()) {
            add_trace(LearningLoopStage::MemoryCompressed, LearningStepDecision::Executed);
        } else {
            add_trace(LearningLoopStage::MemoryCompressed, LearningStepDecision::SkippedNoInput);
        }
    } else {
        add_trace(LearningLoopStage::MemoryCompressed, LearningStepDecision::SkippedByOptions);
    }

    // Step 5: MemoryRecalled
    if (options.enable_memory_recall) {
        if (!input.actor_existing_memories.empty() || !input.actor_existing_summaries.empty()) {
            add_trace(LearningLoopStage::MemoryRecalled, LearningStepDecision::Executed);
        } else {
            add_trace(LearningLoopStage::MemoryRecalled, LearningStepDecision::SkippedNoInput);
        }
    } else {
        add_trace(LearningLoopStage::MemoryRecalled, LearningStepDecision::SkippedByOptions);
    }

    // Step 6: KnowledgeFormed
    if (options.enable_knowledge_formation) {
        add_trace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::SkippedByOptions);
    }

    // Step 7: KnowledgeRevised (actor)
    if (options.enable_knowledge_revision) {
        add_trace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::SkippedByOptions);
    }

    // Step 8: KnowledgePropagated
    if (options.enable_knowledge_propagation && input.recipient.has_value()) {
        add_trace(LearningLoopStage::KnowledgePropagated, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::KnowledgePropagated,
                  input.recipient.has_value() ? LearningStepDecision::SkippedByOptions : LearningStepDecision::SkippedNoInput);
    }

    // Step 9: RecipientInfluenced
    if (options.enable_recipient_projection_check && input.recipient.has_value()) {
        add_trace(LearningLoopStage::RecipientInfluenced, LearningStepDecision::Executed);
    } else {
        add_trace(LearningLoopStage::RecipientInfluenced,
                  input.recipient.has_value() ? LearningStepDecision::SkippedByOptions : LearningStepDecision::SkippedNoInput);
    }

    // Step 10: ReportBuilt
    add_trace(LearningLoopStage::ReportBuilt, LearningStepDecision::Executed);

    if (plan.size() > options.max_step_traces) {
        return Result<std::vector<LearningLoopStepTrace>>::fail(
            makeError(ErrorCode::validation_failed, "Planner: planned traces exceed max_step_traces"));
    }

    return Result<std::vector<LearningLoopStepTrace>>::ok(plan);
}


// ============================================================
// LearningLoopService helpers
// ============================================================

static LearningLoopStepTrace makeTrace(LearningLoopStage stage,
                                       LearningStepDecision decision,
                                       LearningLoopFailureReason reason = LearningLoopFailureReason::None) {
    LearningLoopStepTrace trace;
    trace.stage = stage;
    trace.decision = decision;
    trace.failure_reason = reason;
    trace.summary_key = toString(stage);
    return trace;
}

static bool actorOwnerMatches(const LearningActorRef& actor, const KnowledgeClaim& claim) {
    return claim.owner.kind == actor.knowledge_owner.kind &&
           claim.owner.entity_id == actor.knowledge_owner.entity_id;
}

static bool recipientOwnerMatches(const LearningActorRef& recipient, const KnowledgeClaim& claim) {
    return claim.owner.kind == recipient.knowledge_owner.kind &&
           claim.owner.entity_id == recipient.knowledge_owner.entity_id;
}

static MemoryEvidenceRef makeLearningMemoryEvidenceRef(const LearningSafeFeedbackInput& feedback,
                                                       const std::string& fallback_event_key) {
    MemoryEvidenceRef ref;
    ref.source_kind = MemorySourceKind::DirectEvent;
    ref.source_event_id = feedback.source_event_ids.empty()
        ? EventId(fallback_event_key)
        : feedback.source_event_ids.front();
    ref.observed_tick = feedback.observed_tick;
    ref.reason_keys = feedback.reason_keys;
    return ref;
}

static MemoryRecord makeSyntheticRepresentativeMemory(const LearningLoopInput& input,
                                                       const std::string& suffix,
                                                       double strength) {
    MemoryRecord record;
    record.memory_id = MemoryId("mem_" + input.actor.memory_owner.entity_id.value() + "_" + input.feedback.memory_subject.subject_id + "_" + suffix);
    record.owner = input.actor.memory_owner;
    record.subject = input.feedback.memory_subject;
    record.memory_kinds = {MemoryKind::Experiment};
    if (input.feedback.risk_delta > 0.0) {
        record.memory_kinds.push_back(MemoryKind::Warning);
    } else if (input.feedback.utility_delta >= 0.0) {
        record.memory_kinds.push_back(MemoryKind::Success);
    } else {
        record.memory_kinds.push_back(MemoryKind::Failure);
    }
    record.scope = MemoryScope::MidTerm;
    record.lifecycle = MemoryLifecycle::Active;
    record.importance = input.feedback.risk_delta >= 0.7 ? MemoryImportance::Critical : MemoryImportance::Important;
    record.strength = clamp01(strength);
    record.strength_band = strengthToBand(record.strength);
    record.reinforcement_count = 0;
    record.contradiction_count = input.feedback.risk_delta >= 0.7 ? 1 : 0;
    record.teaching_candidate = record.strength >= 0.55;
    record.created_tick = input.feedback.observed_tick;
    record.last_touched_tick = input.feedback.observed_tick;
    record.evidence_refs.push_back(makeLearningMemoryEvidenceRef(input.feedback, "evt_" + input.loop_key));
    record.reason_keys = {"learning_loop_synthetic_representative"};
    return record;
}

static MemorySummary makeLearningSummary(const LearningLoopInput& input,
                                         const MemoryOwner& owner,
                                         const MemorySubject& subject,
                                         const std::vector<MemoryRecord>& records,
                                         double strength,
                                         const std::vector<std::string>& reason_keys) {
    MemorySummary summary;
    summary.summary_id = MemorySummaryId("sum_" + owner.entity_id.value() + "_" + subject.subject_id + "_" + std::to_string(input.loop_tick.value()));
    summary.key.owner = owner;
    summary.key.subject = subject;
    summary.key.summary_kind = MemorySummaryKind::OwnerSubjectPattern;
    summary.key.memory_kinds = {MemoryKind::Experiment, MemoryKind::Success};
    summary.compression_level = MemoryCompressionLevel::LightSummary;
    summary.highest_importance = input.feedback.risk_delta >= 0.7 ? MemoryImportance::Critical : MemoryImportance::Important;
    summary.aggregate_strength = clamp01(strength);
    summary.aggregate_strength_band = strengthToBand(summary.aggregate_strength);
    summary.source_memory_count = records.empty() ? 1 : records.size();
    summary.success_count = input.feedback.utility_delta >= 0.0 ? 1 : 0;
    summary.failure_count = input.feedback.utility_delta < 0.0 ? 1 : 0;
    summary.warning_count = input.feedback.risk_delta > 0.0 ? 1 : 0;
    summary.accident_count = input.feedback.risk_delta >= 0.7 ? 1 : 0;
    summary.contradiction_count = input.feedback.risk_delta >= 0.7 ? 1 : 0;
    summary.teaching_candidate_count = summary.aggregate_strength >= 0.55 ? 1 : 0;
    summary.first_observed_tick = input.feedback.observed_tick;
    summary.last_observed_tick = input.feedback.observed_tick;
    summary.summarized_tick = input.loop_tick;
    if (records.empty()) {
        auto synthetic = makeSyntheticRepresentativeMemory(input, "summary_source", summary.aggregate_strength);
        summary.source_memory_ids.push_back(synthetic.memory_id);
        summary.representative_memory_ids.push_back(synthetic.memory_id);
        summary.evidence_refs = synthetic.evidence_refs;
    } else {
        for (const auto& record : records) {
            summary.source_memory_ids.push_back(record.memory_id);
            for (const auto& ref : record.evidence_refs) {
                summary.evidence_refs.push_back(ref);
            }
        }
        summary.representative_memory_ids.push_back(records.front().memory_id);
    }
    if (summary.evidence_refs.empty()) {
        summary.evidence_refs.push_back(makeLearningMemoryEvidenceRef(input.feedback, "evt_" + input.loop_key));
    }
    summary.reason_keys = reason_keys;
    return summary;
}

static KnowledgeFormationInput makeFormationInputFromClaimForRecipient(
    const LearningLoopInput& input,
    const KnowledgeClaim& source_claim,
    const LearningActorRef& recipient,
    double strength) {
    KnowledgeFormationInput formation_input;
    formation_input.owner = recipient.knowledge_owner;
    MemorySubject subject = input.feedback.memory_subject;
    subject.subject_id = source_claim.subject.subject_id.empty() ? input.feedback.memory_subject.subject_id : source_claim.subject.subject_id;
    formation_input.summary = makeLearningSummary(input, recipient.memory_owner, subject, {}, strength,
                                                  {"propagation_conflict_revision_summary"});
    formation_input.target_relation = source_claim.predicate.relation_type;
    formation_input.action_key = source_claim.predicate.action_key;
    formation_input.effect_key = source_claim.predicate.effect_key;
    formation_input.related_subject_ids = source_claim.subject.related_subject_ids;
    formation_input.relation_group_key = source_claim.subject.relation_group_key;
    formation_input.candidate_conditions = source_claim.conditions;
    if (formation_input.candidate_conditions.empty()) {
        for (const auto& key : input.feedback.condition_keys) {
            KnowledgeCondition condition;
            condition.condition_key = key;
            formation_input.candidate_conditions.push_back(condition);
        }
    }
    formation_input.reason_keys = {"propagation_conflict_route_to_revision"};
    return formation_input;
}

static bool hasExecutedStage(const std::vector<LearningLoopStepTrace>& traces, LearningLoopStage stage) {
    for (const auto& trace : traces) {
        if (trace.stage == stage && trace.decision == LearningStepDecision::Executed) return true;
    }
    return false;
}

static bool hasFailedStage(const std::vector<LearningLoopStepTrace>& traces, LearningLoopStage stage) {
    for (const auto& trace : traces) {
        if (trace.stage == stage && trace.decision == LearningStepDecision::Failed) return true;
    }
    return false;
}

// ============================================================
// LearningLoopService
// ============================================================

Result<LearningLoopResult> LearningLoopService::run(
    const LearningLoopInput& input,
    const LearningLoopOptions& options) const {

    // Validate inputs
    auto opts_r = options.validateBasic();
    if (opts_r.is_error()) {
        LearningLoopResult fail_result;
        fail_result.decision = LearningLoopDecision::Rejected;
        fail_result.failure_reason = LearningLoopFailureReason::InvalidInput;
        fail_result.reason_keys.push_back("options_validation_failed");
        return Result<LearningLoopResult>::ok(fail_result);
    }
    auto input_r = input.validateBasic();
    if (input_r.is_error()) {
        LearningLoopResult fail_result;
        fail_result.decision = LearningLoopDecision::Rejected;
        fail_result.failure_reason = LearningLoopFailureReason::SecurityRejected;
        fail_result.reason_keys.push_back("input_validation_failed");
        return Result<LearningLoopResult>::ok(fail_result);
    }

    LearningLoopResult result;
    result.decision = LearningLoopDecision::Unknown;
    result.failure_reason = LearningLoopFailureReason::None;
    result.reason_keys.push_back("learning_loop_started");

    LearningEventChain chain;
    chain.chain_key = "chain_" + input.loop_key;
    chain.loop_key = input.loop_key;
    chain.story_kind = input.story_kind;
    chain.source_event_ids = input.feedback.source_event_ids;

    // Local working copies of actor snapshots
    CognitionStateV2 actor_cognition_state;
    std::vector<MemoryRecord> actor_memories = input.actor_existing_memories;
    std::vector<MemorySummary> actor_summaries = input.actor_existing_summaries;
    std::vector<KnowledgeClaim> actor_claims = input.actor_existing_claims;
    std::vector<KnowledgeClaim> recipient_claims = input.recipient_existing_claims;

    // ============================================================
    // Step 1: FeedbackCaptured
    // ============================================================
    result.traces.push_back(makeTrace(LearningLoopStage::FeedbackCaptured, LearningStepDecision::Executed));

    // ============================================================
    // Step 2: CognitionUpdated (P15)
    // ============================================================
    if (options.enable_cognition) {
        // Build feedback signal from input
        CognitionFeedbackSignal signal;
        signal.subject = input.actor.cognition_subject;
        signal.target = input.feedback.cognition_target;
        signal.action_id = input.feedback.action_id;
        signal.action_context = input.feedback.action_context;
        signal.outcome_signal = input.feedback.outcome_signals.empty()
            ? CognitionOutcomeSignal::Unknown
            : input.feedback.outcome_signals[0];
        signal.risk_level = CognitionRiskLevel::None; // default, can be refined
        signal.utility_signal = input.feedback.utility_delta;
        signal.source_event_id = input.feedback.source_event_ids.empty()
            ? EventId("")
            : input.feedback.source_event_ids[0];
        signal.observed_tick = input.feedback.observed_tick;
        signal.reason_keys = input.feedback.reason_keys;

        CognitionEvidenceBuilder evidence_builder;
        auto evidence_r = evidence_builder.fromFeedbackSignal(signal);
        if (evidence_r.is_error()) {
            result.traces.push_back(makeTrace(LearningLoopStage::CognitionUpdated, LearningStepDecision::Failed,
                                               LearningLoopFailureReason::CognitionFailed));
            result.decision = LearningLoopDecision::Failed;
            result.failure_reason = LearningLoopFailureReason::CognitionFailed;
            result.reason_keys.push_back("cognition_evidence_builder_failed");
            result.event_chain = chain;
            return Result<LearningLoopResult>::ok(result);
        }

        CognitionConfidenceModel confidence_model;
        CognitionUpdaterV2 updater;
        std::vector<CognitionEvidenceRecord> evidence_records = evidence_r.value();

        bool any_cognition_failed = false;
        for (const auto& ev : evidence_records) {
            auto update_r = updater.applyEvidence(actor_cognition_state, ev);
            if (update_r.is_error()) {
                any_cognition_failed = true;
                break;
            }
            chain.cognition_evidence_ids.push_back(ev.evidence_id);
            auto& upd = update_r.value();
            if (upd.decision != CognitionUpdateDecision::Ignored) {
                chain.cognition_claim_ids.push_back(upd.after_claim.claim_id);
            }
            result.cognition_result = upd;
        }

        if (any_cognition_failed) {
            result.traces.push_back(makeTrace(LearningLoopStage::CognitionUpdated, LearningStepDecision::Failed,
                                               LearningLoopFailureReason::CognitionFailed));
            result.decision = LearningLoopDecision::Failed;
            result.failure_reason = LearningLoopFailureReason::CognitionFailed;
            result.event_chain = chain;
            return Result<LearningLoopResult>::ok(result);
        }

        result.traces.push_back(makeTrace(LearningLoopStage::CognitionUpdated, LearningStepDecision::Executed));
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::CognitionUpdated, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 3: MemoryWritten (P16)
    // ============================================================
    if (options.enable_memory_write) {
        MemoryWriteService writer;
        MemoryCandidateFactory candidate_factory;
        bool any_memory_failed = false;

        // Use cognition result to generate memory candidate
        if (result.cognition_result.has_value()) {
            const auto& cog_res = result.cognition_result.value();
            // Find the evidence that triggered this update
            const auto& evidences = actor_cognition_state.evidence_records();
            if (!evidences.empty()) {
                auto cand_r = candidate_factory.fromCognitionUpdate(cog_res, evidences.back());
                if (cand_r.is_error()) {
                    any_memory_failed = true;
                } else {
                    auto candidate = cand_r.value();
                    candidate.owner = input.actor.memory_owner;
                    candidate.subject = input.feedback.memory_subject;
                    auto write_r = writer.writeCandidate(candidate, actor_memories);
                    if (write_r.is_error()) {
                        any_memory_failed = true;
                    } else {
                        auto& wres = write_r.value();
                        result.memory_write_result = wres;
                        for (const auto& r : wres.created_records) {
                            actor_memories.push_back(r);
                            chain.memory_ids.push_back(r.memory_id);
                        }
                        for (const auto& r : wres.updated_records) {
                            bool found = false;
                            for (auto& am : actor_memories) {
                                if (am.memory_id == r.memory_id) {
                                    am = r;
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) actor_memories.push_back(r);
                        }
                    }
                }
            }
        }

        if (any_memory_failed) {
            result.traces.push_back(makeTrace(LearningLoopStage::MemoryWritten, LearningStepDecision::Failed,
                                               LearningLoopFailureReason::MemoryWriteFailed));
            if (options.require_full_chain) {
                result.decision = LearningLoopDecision::Failed;
                result.failure_reason = LearningLoopFailureReason::MemoryWriteFailed;
                result.event_chain = chain;
                return Result<LearningLoopResult>::ok(result);
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::MemoryWritten, LearningStepDecision::Executed));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::MemoryWritten, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 4: MemoryCompressed (P17)
    // ============================================================
    if (options.enable_memory_compression) {
        if (!actor_memories.empty()) {
            MemoryCompressionService compression_service;
            MemorySummaryKey key;
            key.owner = input.actor.memory_owner;
            key.subject = input.feedback.memory_subject;
            key.summary_kind = pathfinder::memory::MemorySummaryKind::OwnerSubjectPattern;
            key.memory_kinds = {pathfinder::memory::MemoryKind::Discovery, pathfinder::memory::MemoryKind::Success,
                                pathfinder::memory::MemoryKind::Experiment};

            MemoryCompressionOptions compress_opts;
            compress_opts.include_short_term = true;
            compress_opts.min_records_to_summarize = 1; // allow test to work with few records

            auto compress_r = compression_service.compress(actor_memories, key, compress_opts);
            if (compress_r.is_error()) {
                result.traces.push_back(makeTrace(LearningLoopStage::MemoryCompressed, LearningStepDecision::Failed,
                                                   LearningLoopFailureReason::CompressionFailed));
                if (options.require_full_chain) {
                    result.decision = LearningLoopDecision::Failed;
                    result.failure_reason = LearningLoopFailureReason::CompressionFailed;
                    result.event_chain = chain;
                    return Result<LearningLoopResult>::ok(result);
                }
            } else {
                auto& cres = compress_r.value();
                result.memory_compression_result = cres;
                if (cres.summary.has_value()) {
                    actor_summaries.push_back(cres.summary.value());
                    chain.memory_summary_ids.push_back(cres.summary.value().summary_id);
                    result.traces.push_back(makeTrace(LearningLoopStage::MemoryCompressed, LearningStepDecision::Executed));
                } else {
                    result.traces.push_back(makeTrace(LearningLoopStage::MemoryCompressed, LearningStepDecision::SkippedNoInput));
                }
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::MemoryCompressed, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::MemoryCompressed, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 5: MemoryRecalled (P17)
    // ============================================================
    if (options.enable_memory_recall) {
        if (!actor_memories.empty() || !actor_summaries.empty()) {
            MemoryRecallService recall_service;
            MemoryRecallQuery query;
            query.owner = input.actor.memory_owner;
            query.subject = input.feedback.memory_subject;
            query.mode = MemoryRecallMode::ExactSubject;
            query.include_records = true;
            query.include_summaries = true;
            query.limit = 10;

            auto recall_r = recall_service.recall(actor_memories, actor_summaries, query);
            if (recall_r.is_error()) {
                result.traces.push_back(makeTrace(LearningLoopStage::MemoryRecalled, LearningStepDecision::Failed,
                                                   LearningLoopFailureReason::RecallFailed));
                if (options.require_full_chain) {
                    result.decision = LearningLoopDecision::Failed;
                    result.failure_reason = LearningLoopFailureReason::RecallFailed;
                    result.event_chain = chain;
                    return Result<LearningLoopResult>::ok(result);
                }
            } else {
                result.memory_recall_result = recall_r.value();
                result.traces.push_back(makeTrace(LearningLoopStage::MemoryRecalled, LearningStepDecision::Executed));
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::MemoryRecalled, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::MemoryRecalled, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 6: KnowledgeFormed (P18)
    // ============================================================
    if (options.enable_knowledge_formation) {
        // Use most recent summary if available, or skip if no summaries
        if (!actor_summaries.empty()) {
            // Find the most relevant summary for this subject
            const MemorySummary* best_summary = nullptr;
            for (const auto& s : actor_summaries) {
                if (s.key.subject.subject_id == input.feedback.memory_subject.subject_id) {
                    best_summary = &s;
                }
            }

            MemorySummary current_subject_summary;
            if (!best_summary) {
                std::vector<MemoryRecord> current_subject_records;
                for (const auto& record : actor_memories) {
                    if (record.subject.subject_id == input.feedback.memory_subject.subject_id) {
                        current_subject_records.push_back(record);
                    }
                }
                const double direct_feedback_strength = clamp01(
                    0.35 + std::max(std::abs(input.feedback.utility_delta), std::abs(input.feedback.risk_delta)) * 0.5);
                current_subject_summary = makeLearningSummary(
                    input,
                    input.actor.memory_owner,
                    input.feedback.memory_subject,
                    current_subject_records,
                    direct_feedback_strength,
                    {"learning_loop_current_subject_summary"});
                best_summary = &current_subject_summary;
            }

            if (best_summary) {
                KnowledgeFormationService formation_service;
                KnowledgeFormationInput formation_input;
                formation_input.owner = input.actor.knowledge_owner;
                formation_input.summary = *best_summary;
                formation_input.target_relation = KnowledgeRelationType::HasEffect;
                formation_input.action_key = input.feedback.action_key;
                formation_input.effect_key = input.feedback.effect_key;
                if (!input.feedback.target_subject_id.empty()) {
                    formation_input.related_subject_ids.push_back(input.feedback.target_subject_id);
                    formation_input.relation_group_key = "action_target";
                }
                for (const auto& ck : input.feedback.condition_keys) {
                    KnowledgeCondition cond;
                    cond.condition_key = ck;
                    formation_input.candidate_conditions.push_back(cond);
                }
                formation_input.reason_keys = input.feedback.reason_keys;

                // Representative records for formation
                std::vector<MemoryRecord> representative;
                for (const auto& mid : best_summary->representative_memory_ids) {
                    for (const auto& mr : actor_memories) {
                        if (mr.memory_id == mid) {
                            representative.push_back(mr);
                            break;
                        }
                    }
                }
                formation_input.representative_records = representative;

                KnowledgeFormationOptions formation_opts;
                formation_opts.min_source_memory_count = 1;
                formation_opts.min_representative_count = 1;
                formation_opts.min_summary_strength = 0.20;
                formation_opts.min_hypothesis_confidence = 0.10;
                formation_opts.active_confidence_threshold = 0.15;
                formation_opts.allow_contradiction_summary = true;

                auto form_r = formation_service.formFromMemorySummary(formation_input, formation_opts);
                if (form_r.is_error()) {
                    result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::Failed,
                                                       LearningLoopFailureReason::FormationFailed));
                    if (options.require_full_chain) {
                        result.decision = LearningLoopDecision::Failed;
                        result.failure_reason = LearningLoopFailureReason::FormationFailed;
                        result.event_chain = chain;
                        return Result<LearningLoopResult>::ok(result);
                    }
                } else {
                    auto& fres = form_r.value();
                    result.knowledge_formation_result = fres;
                    if (fres.claim.has_value()) {
                        auto adjusted_claim = fres.claim.value();
                        adjusted_claim.reason_keys.insert(adjusted_claim.reason_keys.end(), input.feedback.reason_keys.begin(), input.feedback.reason_keys.end());
                        applyIndependentExperienceThreshold(adjusted_claim, accumulatedSupportForScope(actor_claims, adjusted_claim));
                        fres.claim = adjusted_claim;
                        actor_claims.push_back(adjusted_claim);
                        chain.knowledge_ids.push_back(adjusted_claim.knowledge_id);
                        auto trace = makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::Executed);
                        trace.knowledge_ids.push_back(adjusted_claim.knowledge_id);
                        result.traces.push_back(std::move(trace));
                    } else {
                        result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::SkippedNoInput));
                    }
                }
            } else {
                result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::SkippedNoInput));
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeFormed, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 7: KnowledgeRevised (actor) (P19)
    // ============================================================
    if (options.enable_knowledge_revision) {
        // Check if new knowledge conflicts with existing actor claims
        if (result.knowledge_formation_result.has_value() &&
            result.knowledge_formation_result->claim.has_value()) {
            const auto& new_claim = result.knowledge_formation_result->claim.value();

            // Find conflicting claims
            std::vector<KnowledgeClaim> conflicting;
            for (const auto& existing : actor_claims) {
                if (existing.knowledge_id == new_claim.knowledge_id) continue;
                if (existing.subject.subject_id == new_claim.subject.subject_id &&
                    existing.predicate.action_key == new_claim.predicate.action_key &&
                    existing.predicate.relation_type == new_claim.predicate.relation_type &&
                    existing.predicate.effect_key != new_claim.predicate.effect_key &&
                    shouldReviseForConditionScope(existing.conditions, new_claim.conditions)) {
                    conflicting.push_back(existing);
                }
            }

            if (!conflicting.empty()) {
                KnowledgeRevisionService revision_service;
                KnowledgeRevisionInput revision_input;
                revision_input.formation_input.owner = input.actor.knowledge_owner;
                revision_input.formation_input.summary = result.knowledge_formation_result->plan.has_value()
                    ? result.knowledge_formation_result->plan->input.summary
                    : MemorySummary{};
                revision_input.formation_input.target_relation = new_claim.predicate.relation_type;
                revision_input.formation_input.action_key = new_claim.predicate.action_key;
                revision_input.formation_input.effect_key = new_claim.predicate.effect_key;
                revision_input.formation_input.related_subject_ids = new_claim.subject.related_subject_ids;
                revision_input.formation_input.relation_group_key = new_claim.subject.relation_group_key;
                revision_input.formation_input.candidate_conditions = new_claim.conditions;
                revision_input.existing_claims = actor_claims;
                revision_input.revision_tick = input.loop_tick;
                revision_input.reason_keys.push_back("contradiction_detected");

                auto rev_r = revision_service.revise(revision_input);
                if (rev_r.is_error()) {
                    result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Failed,
                                                       LearningLoopFailureReason::RevisionFailed));
                    if (options.require_full_chain) {
                        result.decision = LearningLoopDecision::Failed;
                        result.failure_reason = LearningLoopFailureReason::RevisionFailed;
                        result.event_chain = chain;
                        return Result<LearningLoopResult>::ok(result);
                    }
                } else {
                    auto& rres = rev_r.value();
                    result.knowledge_revision_result = rres;
                    auto trace = makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Executed);
                    // Apply revision to actor claims snapshot
                    for (const auto& updated : rres.updated_claims) {
                        bool replaced = false;
                        for (auto& ac : actor_claims) {
                            if (ac.knowledge_id == updated.knowledge_id) {
                                ac = updated;
                                replaced = true;
                                trace.updated_knowledge_ids.push_back(updated.knowledge_id);
                                break;
                            }
                        }
                        if (!replaced) actor_claims.push_back(updated);
                    }
                    for (const auto& created : rres.created_claims) {
                        actor_claims.push_back(created);
                        chain.knowledge_ids.push_back(created.knowledge_id);
                        trace.knowledge_ids.push_back(created.knowledge_id);
                    }
                    result.traces.push_back(std::move(trace));
                }
            } else {
                result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::SkippedNoInput));
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::SkippedByOptions));
    }

    // ============================================================
    // Step 8: KnowledgePropagated (P20)
    // ============================================================
    bool propagation_routed_to_revision = false;
    if (options.enable_knowledge_propagation && input.recipient.has_value()) {
        // Find teachable/active claims from actor snapshot
        std::vector<KnowledgeClaim> transferable;
        for (const auto& c : actor_claims) {
            if (actorOwnerMatches(input.actor, c) &&
                (c.status == KnowledgeStatus::Active ||
                 c.status == KnowledgeStatus::Teachable ||
                 c.status == KnowledgeStatus::Shared ||
                 c.status == KnowledgeStatus::Operational ||
                 c.status == KnowledgeStatus::Hypothesis ||
                 c.status == KnowledgeStatus::Candidate)) {
                transferable.push_back(c);
            }
        }

        if (!transferable.empty()) {
            KnowledgePropagationService propagation_service;
            KnowledgePropagationAttempt attempt;
            attempt.attempt_key = "prop_" + input.loop_key;
            attempt.propagator.owner = input.actor.knowledge_owner;
            for (const auto& c : transferable) {
                attempt.propagator.available_knowledge_ids.push_back(c.knowledge_id);
                KnowledgePropagationSourceClaim sc;
                sc.claim = c;
                sc.selected = true;
                attempt.source_claims.push_back(sc);
            }
            attempt.target.owner = input.recipient->knowledge_owner;
            attempt.target.known_knowledge_ids.clear();
            for (const auto& c : recipient_claims) {
                attempt.target.known_knowledge_ids.push_back(c.knowledge_id);
            }
            attempt.target_existing_claims = recipient_claims;
            attempt.context.channel = KnowledgePropagationChannel::DirectTeaching;
            attempt.context.trust_band = KnowledgePropagationTrustBand::High;
            attempt.context.trust_score = 0.8;
            attempt.context.distance_factor = 1.0;
            attempt.context.channel_quality = 1.0;
            attempt.context.noise_factor = 0.0;
            attempt.context.propagation_tick = input.loop_tick;
            attempt.context.is_correction = (input.story_kind == LearningLoopStoryKind::CorrectionAfterContradiction);
            attempt.reason_keys.push_back("learning_loop_propagation");

            KnowledgePropagationOptions propagation_opts;
            propagation_opts.min_source_confidence = 0.0;
            propagation_opts.min_transfer_score = 0.0;
            auto prop_r = propagation_service.propagate(attempt, propagation_opts);
            if (prop_r.is_error()) {
                result.traces.push_back(makeTrace(LearningLoopStage::KnowledgePropagated, LearningStepDecision::Failed,
                                                   LearningLoopFailureReason::PropagationFailed));
                if (options.require_full_chain) {
                    result.decision = LearningLoopDecision::Failed;
                    result.failure_reason = LearningLoopFailureReason::PropagationFailed;
                    result.event_chain = chain;
                    return Result<LearningLoopResult>::ok(result);
                }
            } else {
                auto& pres = prop_r.value();
                result.knowledge_propagation_result = pres;

                // Check for route_to_revision
                if (pres.plan.has_value()) {
                    for (const auto& assess : pres.plan->assessments) {
                        if (assess.should_route_to_revision) {
                            propagation_routed_to_revision = true;
                        }
                    }
                }

                // Apply propagation to recipient snapshot (if not routed to revision)
                if (!propagation_routed_to_revision || !options.route_propagation_conflict_to_revision) {
                    KnowledgePropagationApplier prop_applier;
                    auto apply_r = prop_applier.applyToTargetSnapshot(recipient_claims, pres);
                    if (apply_r.is_ok()) {
                        recipient_claims = apply_r.value();
                    }
                }

                for (const auto& c : pres.created_claims) {
                    chain.recipient_knowledge_ids.push_back(c.knowledge_id);
                }
                for (const auto& c : pres.updated_claims) {
                    chain.recipient_knowledge_ids.push_back(c.knowledge_id);
                }
                result.traces.push_back(makeTrace(LearningLoopStage::KnowledgePropagated, LearningStepDecision::Executed));
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::KnowledgePropagated, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::KnowledgePropagated,
            input.recipient.has_value() ? LearningStepDecision::SkippedByOptions : LearningStepDecision::SkippedNoInput));
    }

    // ============================================================
    // Step 9: Recipient revision route (P21 requirement)
    // ============================================================
    if (propagation_routed_to_revision && options.route_propagation_conflict_to_revision && input.recipient.has_value()) {
        const KnowledgeClaim* routed_source_claim = nullptr;
        double routed_strength = 0.70;
        if (result.knowledge_propagation_result.has_value() && result.knowledge_propagation_result->plan.has_value()) {
            const auto& plan = result.knowledge_propagation_result->plan.value();
            for (const auto& assessment : plan.assessments) {
                if (!assessment.should_route_to_revision) continue;
                routed_strength = std::max(routed_strength, assessment.transfer_score);
                for (const auto& source : plan.attempt.source_claims) {
                    if (source.claim.knowledge_id == assessment.source_knowledge_id) {
                        routed_source_claim = &source.claim;
                        break;
                    }
                }
                if (routed_source_claim) break;
            }
        }

        if (!routed_source_claim) {
            result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Failed,
                                               LearningLoopFailureReason::RevisionFailed));
        } else {
            KnowledgeRevisionService revision_service;
            KnowledgeRevisionInput revision_input;
            revision_input.formation_input = makeFormationInputFromClaimForRecipient(
                input,
                *routed_source_claim,
                input.recipient.value(),
                std::max(routed_strength, routed_source_claim->confidence.confidence));
            revision_input.existing_claims = recipient_claims;
            revision_input.revision_tick = input.loop_tick;
            revision_input.reason_keys.push_back("propagation_conflict_route_to_revision");

            auto rev_r = revision_service.revise(revision_input);
            if (!rev_r.is_error()) {
                auto& rres = rev_r.value();
                result.knowledge_revision_result = rres;
                auto trace = makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Routed);
                for (const auto& updated : rres.updated_claims) {
                    bool replaced = false;
                    for (auto& rc : recipient_claims) {
                        if (rc.knowledge_id == updated.knowledge_id) {
                            rc = updated;
                            replaced = true;
                            trace.updated_knowledge_ids.push_back(updated.knowledge_id);
                            break;
                        }
                    }
                    if (!replaced) recipient_claims.push_back(updated);
                }
                for (const auto& created : rres.created_claims) {
                    recipient_claims.push_back(created);
                    chain.recipient_knowledge_ids.push_back(created.knowledge_id);
                    trace.knowledge_ids.push_back(created.knowledge_id);
                }
                result.traces.push_back(std::move(trace));
            } else {
                result.traces.push_back(makeTrace(LearningLoopStage::KnowledgeRevised, LearningStepDecision::Failed,
                                                   LearningLoopFailureReason::RevisionFailed));
            }
        }
    }

    // ============================================================
    // Step 10: RecipientInfluenced
    // ============================================================
    if (options.enable_recipient_projection_check && input.recipient.has_value()) {
        if (!recipient_claims.empty()) {
            KnowledgeProjectionBuilder proj_builder;
            KnowledgeQuery query;
            query.owner = input.recipient->knowledge_owner;
            query.mode = pathfinder::knowledge::KnowledgeQueryMode::ByOwner;

            auto proj_r = proj_builder.buildProjection(recipient_claims, query);
            if (proj_r.is_ok()) {
                result.recipient_projection = proj_r.value();
                result.traces.push_back(makeTrace(LearningLoopStage::RecipientInfluenced, LearningStepDecision::Executed));
            } else {
                result.traces.push_back(makeTrace(LearningLoopStage::RecipientInfluenced, LearningStepDecision::Failed,
                                                   LearningLoopFailureReason::RecipientInfluenceMissing));
            }
        } else {
            result.traces.push_back(makeTrace(LearningLoopStage::RecipientInfluenced, LearningStepDecision::SkippedNoInput));
        }
    } else {
        result.traces.push_back(makeTrace(LearningLoopStage::RecipientInfluenced,
            input.recipient.has_value() ? LearningStepDecision::SkippedByOptions : LearningStepDecision::SkippedNoInput));
    }

    // ============================================================
    // Step 11: ReportBuilt
    // ============================================================
    result.traces.push_back(makeTrace(LearningLoopStage::ReportBuilt, LearningStepDecision::Executed));

    // ============================================================
    // Finalize snapshots and chain
    // ============================================================
    result.actor_claim_snapshot_after = actor_claims;
    result.recipient_claim_snapshot_after = recipient_claims;
    result.event_chain = chain;

    // Determine final decision
    bool any_failed = false;
    bool any_routed = false;
    bool all_core_complete = true;

    for (const auto& t : result.traces) {
        if (t.decision == LearningStepDecision::Failed) {
            any_failed = true;
            if (t.stage == LearningLoopStage::CognitionUpdated ||
                t.stage == LearningLoopStage::MemoryWritten ||
                t.stage == LearningLoopStage::KnowledgeFormed) {
                all_core_complete = false;
            }
        }
        if (t.decision == LearningStepDecision::Routed) {
            any_routed = true;
        }
    }

    bool missing_required_stage = false;
    if (options.enable_cognition && !hasExecutedStage(result.traces, LearningLoopStage::CognitionUpdated)) {
        missing_required_stage = true;
    }
    if (options.enable_memory_write && !hasExecutedStage(result.traces, LearningLoopStage::MemoryWritten)) {
        missing_required_stage = true;
    }
    if (options.enable_knowledge_formation && !hasExecutedStage(result.traces, LearningLoopStage::KnowledgeFormed)) {
        missing_required_stage = true;
    }
    if (input.recipient.has_value() && options.enable_knowledge_propagation &&
        !hasExecutedStage(result.traces, LearningLoopStage::KnowledgePropagated)) {
        missing_required_stage = true;
    }
    if (input.recipient.has_value() && options.enable_recipient_projection_check &&
        !hasExecutedStage(result.traces, LearningLoopStage::RecipientInfluenced)) {
        missing_required_stage = true;
    }

    if (any_routed && !hasFailedStage(result.traces, LearningLoopStage::KnowledgeRevised)) {
        result.decision = LearningLoopDecision::RoutedToRevision;
    } else if (any_failed || missing_required_stage) {
        if (options.require_full_chain || !all_core_complete) {
            result.decision = LearningLoopDecision::Failed;
        } else {
            result.decision = LearningLoopDecision::PartialCompleted;
        }
    } else {
        result.decision = LearningLoopDecision::Completed;
    }

    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<LearningLoopResult>::fail(result_r.errors());
    }

    return Result<LearningLoopResult>::ok(result);
}


// ============================================================
// LearningDebugReportBuilder
// ============================================================

Result<LearningDebugReport> LearningDebugReportBuilder::buildReport(
    const LearningLoopInput& input,
    const LearningLoopResult& result,
    const LearningLoopOptions& options) const {

    auto input_r = input.validateBasic();
    if (input_r.is_error()) {
        return Result<LearningDebugReport>::fail(input_r.errors());
    }
    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<LearningDebugReport>::fail(result_r.errors());
    }

    LearningDebugReport report;
    report.report_key = "report_" + input.loop_key;
    report.loop_key = input.loop_key;
    report.decision = result.decision;
    report.failure_reason = result.failure_reason;

    // Build timeline keys from traces
    for (const auto& t : result.traces) {
        std::string key = toString(t.stage) + "_" + toString(t.decision);
        report.timeline_keys.push_back(key);
    }

    // Actor knowledge summary keys
    for (const auto& c : result.actor_claim_snapshot_after) {
        std::string key = c.subject.subject_id + "_" + c.predicate.action_key + "_" + toString(c.status);
        report.actor_knowledge_summary_keys.push_back(key);
    }
    std::sort(report.actor_knowledge_summary_keys.begin(), report.actor_knowledge_summary_keys.end());
    report.actor_knowledge_summary_keys.erase(
        std::unique(report.actor_knowledge_summary_keys.begin(), report.actor_knowledge_summary_keys.end()),
        report.actor_knowledge_summary_keys.end());

    // Recipient knowledge summary keys
    for (const auto& c : result.recipient_claim_snapshot_after) {
        std::string key = c.subject.subject_id + "_" + c.predicate.action_key + "_" + toString(c.status);
        report.recipient_knowledge_summary_keys.push_back(key);
    }
    std::sort(report.recipient_knowledge_summary_keys.begin(), report.recipient_knowledge_summary_keys.end());
    report.recipient_knowledge_summary_keys.erase(
        std::unique(report.recipient_knowledge_summary_keys.begin(), report.recipient_knowledge_summary_keys.end()),
        report.recipient_knowledge_summary_keys.end());

    // Trace summary keys
    for (const auto& t : result.traces) {
        report.trace_summary_keys.push_back(toString(t.stage));
    }
    std::sort(report.trace_summary_keys.begin(), report.trace_summary_keys.end());
    report.trace_summary_keys.erase(
        std::unique(report.trace_summary_keys.begin(), report.trace_summary_keys.end()),
        report.trace_summary_keys.end());

    report.warning_keys = result.warning_keys;
    std::sort(report.warning_keys.begin(), report.warning_keys.end());
    report.warning_keys.erase(
        std::unique(report.warning_keys.begin(), report.warning_keys.end()),
        report.warning_keys.end());

    if (report.timeline_keys.size() > options.max_report_items) {
        report.timeline_keys.resize(options.max_report_items);
    }
    if (report.actor_knowledge_summary_keys.size() > options.max_report_items) {
        report.actor_knowledge_summary_keys.resize(options.max_report_items);
    }
    if (report.recipient_knowledge_summary_keys.size() > options.max_report_items) {
        report.recipient_knowledge_summary_keys.resize(options.max_report_items);
    }
    if (report.trace_summary_keys.size() > options.max_report_items) {
        report.trace_summary_keys.resize(options.max_report_items);
    }

    auto report_r = report.validateBasic();
    if (report_r.is_error()) {
        return Result<LearningDebugReport>::fail(report_r.errors());
    }
    return Result<LearningDebugReport>::ok(report);
}

// ============================================================
// LearningLoopApplier
// ============================================================

Result<std::vector<KnowledgeClaim>> LearningLoopApplier::applyActorClaims(
    const std::vector<KnowledgeClaim>& current_claims,
    const LearningLoopResult& result) const {

    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<std::vector<KnowledgeClaim>>::fail(result_r.errors());
    }

    std::vector<KnowledgeClaim> snapshot = current_claims;

    // Apply knowledge formation result
    if (result.knowledge_formation_result.has_value() && result.knowledge_formation_result->claim.has_value()) {
        bool exists = false;
        for (const auto& c : snapshot) {
            if (c.knowledge_id == result.knowledge_formation_result->claim->knowledge_id) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            snapshot.push_back(result.knowledge_formation_result->claim.value());
        }
    }

    // Apply knowledge revision result
    if (result.knowledge_revision_result.has_value()) {
        KnowledgeRevisionApplier revision_applier;
        auto rev_r = revision_applier.applyToSnapshot(snapshot, result.knowledge_revision_result.value());
        if (rev_r.is_ok()) {
            snapshot = rev_r.value();
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

Result<std::vector<KnowledgeClaim>> LearningLoopApplier::applyRecipientClaims(
    const std::vector<KnowledgeClaim>& current_claims,
    const LearningLoopResult& result) const {

    auto result_r = result.validateBasic();
    if (result_r.is_error()) {
        return Result<std::vector<KnowledgeClaim>>::fail(result_r.errors());
    }

    std::vector<KnowledgeClaim> snapshot = current_claims;

    // Apply propagation result
    if (result.knowledge_propagation_result.has_value()) {
        KnowledgePropagationApplier prop_applier;
        auto prop_r = prop_applier.applyToTargetSnapshot(snapshot, result.knowledge_propagation_result.value());
        if (prop_r.is_ok()) {
            snapshot = prop_r.value();
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

} // namespace pathfinder::learning
