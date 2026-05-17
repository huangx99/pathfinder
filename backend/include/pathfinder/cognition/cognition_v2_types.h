#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/cognition/types.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::cognition {

// ============================================================
// Enums
// ============================================================

enum class CognitionSubjectKind {
    Unknown,
    Actor,
    Agent,
    Group,
    Tribe,
    Civilization,
    TestOnly
};

std::string toString(CognitionSubjectKind kind);
pathfinder::foundation::Result<CognitionSubjectKind> cognitionSubjectKindFromString(const std::string& str);

enum class CognitionTargetKind {
    Unknown,
    ObjectDefinition, // hidden_truth: safe enum value
    WorldObject,
    ObjectCategory,
    AgentSpecies,
    EnvironmentFeature,
    TestOnly
};

std::string toString(CognitionTargetKind kind);
pathfinder::foundation::Result<CognitionTargetKind> cognitionTargetKindFromString(const std::string& str);

enum class CognitionAspect {
    Unknown,
    Edibility,
    Usability,
    ConsumptionEffect,
    UseEffect,
    Risk,
    Utility
};

std::string toString(CognitionAspect aspect);
pathfinder::foundation::Result<CognitionAspect> cognitionAspectFromString(const std::string& str);

enum class CognitionBeliefPolarity {
    Unknown,
    Positive,
    Negative,
    Mixed
};

std::string toString(CognitionBeliefPolarity polarity);
pathfinder::foundation::Result<CognitionBeliefPolarity> cognitionBeliefPolarityFromString(const std::string& str);

enum class CognitionEvidenceSupport {
    Unknown,
    Supports,
    Refutes,
    Neutral,
    Conflicts
};

std::string toString(CognitionEvidenceSupport support);
pathfinder::foundation::Result<CognitionEvidenceSupport> cognitionEvidenceSupportFromString(const std::string& str);

enum class CognitionEvidenceSourceKind {
    Unknown,
    DirectActionFeedback,
    ObservedEvent,
    StateChangeObservation,
    TaughtByOther,
    ImportedSafeProjection,
    TestOnly
};

std::string toString(CognitionEvidenceSourceKind kind);
pathfinder::foundation::Result<CognitionEvidenceSourceKind> cognitionEvidenceSourceKindFromString(const std::string& str);

enum class CognitionActionContextKind {
    Unknown,
    Eat,
    Use,
    Touch,
    Combine,
    Observe,
    TestOnly
};

std::string toString(CognitionActionContextKind kind);
pathfinder::foundation::Result<CognitionActionContextKind> cognitionActionContextKindFromString(const std::string& str);

enum class CognitionOutcomeSignal {
    Unknown,
    NeedImproved,
    NeedWorsened,
    HealthImproved,
    HealthWorsened,
    DamageTaken,
    ObjectConsumed,
    ToolActivated,
    NewObjectProduced,
    NoVisibleEffect,
    DangerObserved,
    TestOnly
};

std::string toString(CognitionOutcomeSignal signal);
pathfinder::foundation::Result<CognitionOutcomeSignal> cognitionOutcomeSignalFromString(const std::string& str);

enum class CognitionRiskLevel {
    Unknown,
    None,
    Low,
    Medium,
    High,
    Critical
};

std::string toString(CognitionRiskLevel level);
pathfinder::foundation::Result<CognitionRiskLevel> cognitionRiskLevelFromString(const std::string& str);

enum class CognitionConfidenceBand {
    Unknown,
    Untrusted,
    Hypothesis,
    Actionable,
    Reliable,
    Teachable
};

std::string toString(CognitionConfidenceBand band);
pathfinder::foundation::Result<CognitionConfidenceBand> cognitionConfidenceBandFromString(const std::string& str);

enum class CognitionUpdateDecision {
    Unknown,
    Created,
    Reinforced,
    Weakened,
    Conflicted,
    Ignored
};

std::string toString(CognitionUpdateDecision decision);
pathfinder::foundation::Result<CognitionUpdateDecision> cognitionUpdateDecisionFromString(const std::string& str);

enum class CognitionQueryMode {
    Unknown,
    ExactTarget,
    IncludeTargetKindFallback,
    BestActionable,
    TeachableCandidates
};

std::string toString(CognitionQueryMode mode);
pathfinder::foundation::Result<CognitionQueryMode> cognitionQueryModeFromString(const std::string& str);

// ============================================================
// ID Types
// ============================================================

struct CognitionEvidenceRecordIdTag {};
using CognitionEvidenceRecordId = pathfinder::foundation::StrongId<CognitionEvidenceRecordIdTag>;

struct CognitionClaimV2IdTag {};
using CognitionClaimV2Id = pathfinder::foundation::StrongId<CognitionClaimV2IdTag>;

// ============================================================
// DTOs
// ============================================================

struct CognitionSubject {
    CognitionSubjectKind kind = CognitionSubjectKind::Unknown;
    pathfinder::foundation::EntityId subject_id;
    std::optional<pathfinder::foundation::EntityId> owner_group_id;

    bool operator==(const CognitionSubject& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionTarget {
    CognitionTargetKind kind = CognitionTargetKind::Unknown;
    std::string target_id;
    std::optional<std::string> category_id;
    std::string public_label_key;

    bool operator==(const CognitionTarget& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionClaimKeyV2 {
    CognitionSubject subject;
    CognitionTarget target;
    pathfinder::foundation::ActionId action_id;
    CognitionActionContextKind action_context = CognitionActionContextKind::Unknown;
    CognitionAspect aspect = CognitionAspect::Unknown;

    bool operator==(const CognitionClaimKeyV2& other) const = default;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionEvidenceRecord {
    CognitionEvidenceRecordId evidence_id;
    CognitionClaimKeyV2 key;
    CognitionEvidenceSourceKind source_kind = CognitionEvidenceSourceKind::Unknown;
    CognitionEvidenceSupport support = CognitionEvidenceSupport::Unknown;
    CognitionOutcomeSignal outcome_signal = CognitionOutcomeSignal::Unknown;
    CognitionRiskLevel observed_risk = CognitionRiskLevel::Unknown;
    double confidence_weight = 0.0;
    double utility_signal = 0.0;
    pathfinder::foundation::EventId source_event_id;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionClaimV2 {
    CognitionClaimV2Id claim_id;
    CognitionClaimKeyV2 key;
    CognitionBeliefPolarity polarity = CognitionBeliefPolarity::Unknown;
    double confidence = 0.0;
    CognitionConfidenceBand confidence_band = CognitionConfidenceBand::Unknown;
    CognitionRiskLevel risk_level = CognitionRiskLevel::Unknown;
    double utility_score = 0.0;
    size_t evidence_count = 0;
    size_t supporting_evidence_count = 0;
    size_t refuting_evidence_count = 0;
    bool conflicted = false;
    std::optional<CognitionEvidenceRecordId> first_evidence_id;
    std::optional<CognitionEvidenceRecordId> last_evidence_id;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionFeedbackSignal {
    CognitionSubject subject;
    CognitionTarget target;
    pathfinder::foundation::ActionId action_id;
    CognitionActionContextKind action_context = CognitionActionContextKind::Unknown;
    CognitionOutcomeSignal outcome_signal = CognitionOutcomeSignal::Unknown;
    CognitionRiskLevel risk_level = CognitionRiskLevel::Unknown;
    double utility_signal = 0.0;
    pathfinder::foundation::EventId source_event_id;
    pathfinder::foundation::Tick observed_tick;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Update DTOs
// ============================================================

struct CognitionConfidenceModelOptions {
    double initial_direct_feedback_confidence = 0.35;
    double reinforce_multiplier = 0.65;
    double refute_multiplier = 0.75;
    double conflict_penalty = 0.20;
    double min_confidence = 0.0;
    double max_confidence = 1.0;
    double actionable_threshold = 0.50;
    double reliable_threshold = 0.70;
    double teachable_threshold = 0.85;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionUpdateRequest {
    std::optional<CognitionClaimV2> existing_claim;
    CognitionEvidenceRecord evidence;
    CognitionConfidenceModelOptions options;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionUpdateResult {
    CognitionUpdateDecision decision = CognitionUpdateDecision::Unknown;
    std::optional<CognitionClaimV2> before_claim;
    CognitionClaimV2 after_claim;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// Query DTOs
// ============================================================

struct CognitionQuery {
    CognitionSubject subject;
    std::optional<CognitionTarget> target;
    std::optional<pathfinder::foundation::ActionId> action_id;
    std::optional<CognitionActionContextKind> action_context;
    std::optional<CognitionAspect> aspect;
    CognitionQueryMode mode = CognitionQueryMode::ExactTarget;
    double min_confidence = 0.0;
    bool include_conflicted = true;
    size_t max_results = 50;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CognitionQueryResult {
    CognitionQuery query;
    std::vector<CognitionClaimV2> claims;
    std::optional<CognitionClaimV2> best_claim;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// ID Generation Helpers
// ============================================================

CognitionEvidenceRecordId makeEvidenceRecordId(
    const pathfinder::foundation::EventId& source_event_id,
    CognitionAspect aspect,
    size_t index);

CognitionClaimV2Id makeClaimV2Id(
    const pathfinder::foundation::EntityId& subject_id,
    const std::string& target_id,
    const pathfinder::foundation::ActionId& action_id,
    CognitionAspect aspect);

// ============================================================
// Hidden Truth Validation
// ============================================================

bool containsHiddenTruthKey(const std::string& text);
bool containsHiddenTruthKey(const std::vector<std::string>& texts);

} // namespace pathfinder::cognition
