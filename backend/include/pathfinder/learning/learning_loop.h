#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/cognition/cognition_v2_types.h"
#include "pathfinder/cognition/cognition_v2_state.h"
#include "pathfinder/cognition/cognition_confidence.h"
#include "pathfinder/cognition/cognition_evidence_builder.h"
#include "pathfinder/memory/memory_record.h"
#include "pathfinder/memory/memory_summary.h"
#include "pathfinder/memory/memory_compression.h"
#include "pathfinder/memory/memory_recall.h"
#include "pathfinder/memory/memory_writer.h"
#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_formation.h"
#include "pathfinder/knowledge/knowledge_projection.h"
#include "pathfinder/knowledge/knowledge_revision.h"
#include "pathfinder/knowledge/knowledge_propagation.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::learning {

// ============================================================
// Enums
// ============================================================

enum class LearningLoopStoryKind {
    Unknown,
    DirectDiscovery,
    RepeatedLearning,
    CorrectionAfterContradiction,
    TeachingToRecipient,
    FullLearningLoop,
    TestOnly
};

std::string toString(LearningLoopStoryKind kind);
pathfinder::foundation::Result<LearningLoopStoryKind> learningLoopStoryKindFromString(const std::string& str);

enum class LearningLoopStage {
    Unknown,
    FeedbackCaptured,
    CognitionUpdated,
    MemoryWritten,
    MemoryCompressed,
    MemoryRecalled,
    KnowledgeFormed,
    KnowledgeRevised,
    KnowledgePropagated,
    RecipientInfluenced,
    ReportBuilt,
    Completed,
    Failed,
    TestOnly
};

std::string toString(LearningLoopStage stage);
pathfinder::foundation::Result<LearningLoopStage> learningLoopStageFromString(const std::string& str);

enum class LearningLoopDecision {
    Unknown,
    Completed,
    PartialCompleted,
    Skipped,
    RoutedToRevision,
    Rejected,
    Failed,
    TestOnly
};

std::string toString(LearningLoopDecision decision);
pathfinder::foundation::Result<LearningLoopDecision> learningLoopDecisionFromString(const std::string& str);

enum class LearningLoopFailureReason {
    Unknown,
    None,
    InvalidInput,
    MissingFeedback,
    SecurityRejected,
    CognitionFailed,
    MemoryWriteFailed,
    CompressionFailed,
    RecallFailed,
    FormationFailed,
    RevisionFailed,
    PropagationFailed,
    RecipientInfluenceMissing,
    ReportFailed,
    TestOnly
};

std::string toString(LearningLoopFailureReason reason);
pathfinder::foundation::Result<LearningLoopFailureReason> learningLoopFailureReasonFromString(const std::string& str);

enum class LearningStepDecision {
    Unknown,
    Executed,
    SkippedByOptions,
    SkippedNoInput,
    Routed,
    Rejected,
    Failed,
    TestOnly
};

std::string toString(LearningStepDecision decision);
pathfinder::foundation::Result<LearningStepDecision> learningStepDecisionFromString(const std::string& str);

// ============================================================
// Hidden Truth Guard
// ============================================================

std::vector<std::string> learningForbiddenKeys();
bool containsLearningForbiddenKey(const std::string& text);
bool containsLearningForbiddenKey(const std::vector<std::string>& values);

// ============================================================
// DTOs
// ============================================================

struct LearningActorRef {
    pathfinder::knowledge::KnowledgeOwner knowledge_owner;
    pathfinder::memory::MemoryOwner memory_owner;
    pathfinder::cognition::CognitionSubject cognition_subject;
    std::string display_key;
    std::vector<std::string> safe_tags;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningSafeFeedbackInput {
    std::string feedback_key;
    pathfinder::cognition::CognitionTarget cognition_target;
    pathfinder::memory::MemorySubject memory_subject;
    pathfinder::knowledge::KnowledgeSubject knowledge_subject;
    pathfinder::cognition::CognitionActionContextKind action_context;
    pathfinder::foundation::ActionId action_id;
    std::string action_key;
    std::string target_subject_id;
    std::string target_subject_type_key;
    std::vector<pathfinder::cognition::CognitionOutcomeSignal> outcome_signals;
    std::string effect_key;
    std::vector<std::string> condition_keys;
    double utility_delta = 0.0;
    double risk_delta = 0.0;
    pathfinder::foundation::Tick observed_tick;
    std::vector<pathfinder::foundation::EventId> source_event_ids;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningLoopOptions {
    bool allow_test_only = false;
    bool enable_cognition = true;
    bool enable_memory_write = true;
    bool enable_memory_compression = true;
    bool enable_memory_recall = true;
    bool enable_knowledge_formation = true;
    bool enable_knowledge_revision = true;
    bool enable_knowledge_propagation = true;
    bool enable_recipient_projection_check = true;
    bool require_full_chain = true;
    bool route_propagation_conflict_to_revision = true;
    size_t max_step_traces = 64;
    size_t max_report_items = 128;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningLoopInput {
    std::string loop_key;
    LearningLoopStoryKind story_kind = LearningLoopStoryKind::Unknown;
    LearningActorRef actor;
    std::optional<LearningActorRef> recipient;
    LearningSafeFeedbackInput feedback;
    std::vector<pathfinder::memory::MemoryRecord> actor_existing_memories;
    std::vector<pathfinder::memory::MemorySummary> actor_existing_summaries;
    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_existing_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_existing_claims;
    pathfinder::foundation::Tick loop_tick;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningEventChain {
    std::string chain_key;
    std::string loop_key;
    LearningLoopStoryKind story_kind = LearningLoopStoryKind::Unknown;
    std::vector<pathfinder::foundation::EventId> source_event_ids;
    std::vector<pathfinder::cognition::CognitionEvidenceRecordId> cognition_evidence_ids;
    std::vector<pathfinder::cognition::CognitionClaimV2Id> cognition_claim_ids;
    std::vector<pathfinder::foundation::MemoryId> memory_ids;
    std::vector<pathfinder::memory::MemorySummaryId> memory_summary_ids;
    std::vector<pathfinder::foundation::KnowledgeId> knowledge_ids;
    std::vector<pathfinder::foundation::KnowledgeId> recipient_knowledge_ids;
    std::vector<std::string> chain_reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningLoopStepTrace {
    LearningLoopStage stage = LearningLoopStage::Unknown;
    LearningStepDecision decision = LearningStepDecision::Unknown;
    LearningLoopFailureReason failure_reason = LearningLoopFailureReason::None;
    std::string summary_key;
    std::vector<pathfinder::foundation::MemoryId> memory_ids;
    std::vector<pathfinder::memory::MemorySummaryId> memory_summary_ids;
    std::vector<pathfinder::foundation::KnowledgeId> knowledge_ids;
    std::vector<pathfinder::foundation::KnowledgeId> updated_knowledge_ids;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningLoopResult {
    LearningLoopDecision decision = LearningLoopDecision::Unknown;
    LearningLoopFailureReason failure_reason = LearningLoopFailureReason::Unknown;
    std::optional<LearningEventChain> event_chain;
    std::vector<LearningLoopStepTrace> traces;

    std::optional<pathfinder::cognition::CognitionUpdateResult> cognition_result;
    std::optional<pathfinder::memory::MemoryWriteResult> memory_write_result;
    std::optional<pathfinder::memory::MemoryCompressionResult> memory_compression_result;
    std::optional<pathfinder::memory::MemoryRecallResult> memory_recall_result;
    std::optional<pathfinder::knowledge::KnowledgeFormationResult> knowledge_formation_result;
    std::optional<pathfinder::knowledge::KnowledgeRevisionResult> knowledge_revision_result;
    std::optional<pathfinder::knowledge::KnowledgePropagationResult> knowledge_propagation_result;
    std::optional<pathfinder::knowledge::KnowledgeProjection> recipient_projection;

    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_claim_snapshot_after;
    std::vector<pathfinder::knowledge::KnowledgeClaim> recipient_claim_snapshot_after;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    bool ok() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct LearningDebugReport {
    std::string report_key;
    std::string loop_key;
    LearningLoopDecision decision = LearningLoopDecision::Unknown;
    LearningLoopFailureReason failure_reason = LearningLoopFailureReason::Unknown;
    std::vector<std::string> timeline_keys;
    std::vector<std::string> actor_knowledge_summary_keys;
    std::vector<std::string> recipient_knowledge_summary_keys;
    std::vector<std::string> trace_summary_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Services
// ============================================================

class LearningLoopPlanner {
public:
    pathfinder::foundation::Result<std::vector<LearningLoopStepTrace>> plan(
        const LearningLoopInput& input,
        const LearningLoopOptions& options = {}) const;
};

class LearningLoopService {
public:
    pathfinder::foundation::Result<LearningLoopResult> run(
        const LearningLoopInput& input,
        const LearningLoopOptions& options = {}) const;
};

class LearningDebugReportBuilder {
public:
    pathfinder::foundation::Result<LearningDebugReport> buildReport(
        const LearningLoopInput& input,
        const LearningLoopResult& result,
        const LearningLoopOptions& options = {}) const;
};

class LearningLoopApplier {
public:
    pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> applyActorClaims(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& current_claims,
        const LearningLoopResult& result) const;

    pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> applyRecipientClaims(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& current_claims,
        const LearningLoopResult& result) const;
};

} // namespace pathfinder::learning
