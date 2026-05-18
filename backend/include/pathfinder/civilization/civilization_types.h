#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::civilization {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::EventId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::Result;
using pathfinder::foundation::StateVersion;
using pathfinder::foundation::Tick;
using pathfinder::foundation::TribeId;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

enum class CivilizationStage {
    Unknown,
    Awakening,
    Foraging,
    FireKeeping,
    ToolUsing,
    OrganizedTribe,
    SymbolicCulture,
    ProtoCivilization,
    TestOnly
};

std::string toString(CivilizationStage stage);
Result<CivilizationStage> civilizationStageFromString(const std::string& str);

enum class CapabilityType {
    Unknown,
    IdentifyEdible,
    IdentifyDanger,
    SafeForaging,
    TeachingPractice,
    BasicToolUse,
    FireKeeping,
    FireDefense,
    OrganizedRetreat,
    ConflictDeescalation,
    ResourceDefense,
    TaskAssignment,
    LongTermMemory,
    SymbolRecording,
    TestOnly
};

std::string toString(CapabilityType type);
Result<CapabilityType> capabilityTypeFromString(const std::string& str);

enum class CapabilityMaturityState {
    Unknown,
    Candidate,
    Emerging,
    Stable,
    Institutionalized,
    Degraded,
    Lost,
    TestOnly
};

std::string toString(CapabilityMaturityState state);
Result<CapabilityMaturityState> capabilityMaturityStateFromString(const std::string& str);

enum class CapabilityUsabilityState {
    Unknown,
    Usable,
    BlockedByResource,
    BlockedByEnvironment,
    BlockedByCohesion,
    BlockedByConflict,
    Suspended,
    TestOnly
};

std::string toString(CapabilityUsabilityState state);
Result<CapabilityUsabilityState> capabilityUsabilityStateFromString(const std::string& str);

enum class CapabilityChangeReason {
    Unknown,
    NewEvidence,
    RequirementMet,
    RequirementMissing,
    RepeatedSuccess,
    RepeatedFailure,
    PropagationImproved,
    PropagationFailed,
    ResourceChanged,
    PopulationChanged,
    ConflictChanged,
    MemoryDecayed,
    TestOnly
};

std::string toString(CapabilityChangeReason reason);
Result<CapabilityChangeReason> capabilityChangeReasonFromString(const std::string& str);

enum class CapabilityEffectTarget {
    Unknown,
    ActionAvailability,
    Risk,
    Knowledge,
    Propagation,
    TribeCohesion,
    CombatCoordination,
    ConflictResolution,
    CivilizationStage,
    AiSignal,
    TestOnly
};

std::string toString(CapabilityEffectTarget target);
Result<CapabilityEffectTarget> capabilityEffectTargetFromString(const std::string& str);

enum class EffectOperationType {
    Unknown,
    Add,
    Multiply,
    UnlockCandidate,
    EnableAction,
    AddObservationFeature,
    TestOnly
};

std::string toString(EffectOperationType op);
Result<EffectOperationType> effectOperationTypeFromString(const std::string& str);

// ---------------------------------------------------------------------------
// Forbidden keys
// ---------------------------------------------------------------------------

bool containsCivilizationForbiddenKey(const std::string& key);
bool containsCivilizationForbiddenKey(const std::vector<std::string>& keys);

// ---------------------------------------------------------------------------
// DTOs
// ---------------------------------------------------------------------------

struct CivilizationProgress {
    double candidate_score{0.0};
    double emerging_score{0.0};
    double stable_score{0.0};
    double institutionalized_score{0.0};
    int success_count{0};
    int failure_count{0};
    std::optional<Tick> last_success_tick;
    std::optional<Tick> last_failure_tick;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CapabilityUnlockCondition {
    std::string condition_key;
    std::string condition_type;
    double required_score{0.0};
    double current_score{0.0};
    bool met{false};
    std::vector<EventId> source_evidence_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CapabilityEvidenceBundle {
    std::vector<KnowledgeId> knowledge_ids;
    std::vector<EventId> propagation_event_ids;
    std::vector<EventId> practice_event_ids;
    std::vector<EventId> conflict_event_ids;
    StateVersion tribe_state_version;
    double confidence{0.0};
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CapabilityEffectDefinition {
    std::string effect_key;
    CapabilityEffectTarget target{CapabilityEffectTarget::Unknown};
    EffectOperationType operation{EffectOperationType::Unknown};
    std::string value_key;
    std::optional<double> value_number;
    int priority{0};
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationCapability {
    CapabilityType capability_type{CapabilityType::Unknown};
    std::string display_key;
    std::string domain_key;
    int priority{0};
    std::vector<CapabilityUnlockCondition> required_conditions;
    std::vector<CapabilityEffectDefinition> effect_definitions;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationCapabilityState {
    TribeId tribe_id;
    CapabilityType capability_type{CapabilityType::Unknown};
    CapabilityMaturityState maturity{CapabilityMaturityState::Unknown};
    CapabilityUsabilityState usability{CapabilityUsabilityState::Unknown};
    CivilizationProgress progress;
    double stability{0.0};
    double coverage{0.0};
    CapabilityEvidenceBundle evidence;
    std::vector<std::string> active_effect_keys;
    std::vector<CapabilityChangeReason> blocked_reasons;
    Tick last_changed_tick;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CapabilityEffectDraft {
    EventId draft_id;
    TribeId tribe_id;
    CapabilityType capability_type{CapabilityType::Unknown};
    std::string effect_key;
    CapabilityEffectTarget target{CapabilityEffectTarget::Unknown};
    EffectOperationType operation{EffectOperationType::Unknown};
    std::string value_key;
    std::optional<double> value_number;
    double strength{0.0};
    std::vector<EventId> source_evidence_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationState {
    TribeId tribe_id;
    CivilizationStage stage{CivilizationStage::Unknown};
    double stage_confidence{0.0};
    std::vector<CivilizationCapabilityState> capabilities;
    std::vector<CapabilityEffectDraft> active_effects;
    std::vector<CapabilityType> blocked_capabilities;
    Tick last_resolved_tick;
    StateVersion version;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CapabilityRequirementSnapshot {
    Tick evaluated_tick;
    double total_score{0.0};
    std::vector<std::string> met_condition_keys;
    std::vector<std::string> missing_condition_keys;
    std::vector<std::string> failed_requirements;
    std::vector<std::string> warning_requirements;
    std::vector<EventId> source_candidate_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationCapabilityCandidate {
    EventId candidate_id;
    TribeId tribe_id;
    CapabilityType capability_type{CapabilityType::Unknown};
    double readiness{0.0};
    CapabilityRequirementSnapshot requirement_snapshot;
    CapabilityEvidenceBundle evidence;
    std::vector<std::string> recommended_next_steps;
    bool can_promote{false};
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationStateChangeDraft {
    EventId change_id;
    TribeId tribe_id;
    std::optional<CapabilityType> capability_type;
    std::optional<CivilizationStage> stage_before;
    std::optional<CivilizationStage> stage_after;
    std::optional<CapabilityMaturityState> maturity_before;
    std::optional<CapabilityMaturityState> maturity_after;
    std::optional<CapabilityUsabilityState> usability_before;
    std::optional<CapabilityUsabilityState> usability_after;
    std::vector<CapabilityChangeReason> reasons;
    std::vector<EventId> evidence_ids;
    std::string deterministic_key;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationProjection {
    TribeId tribe_id;
    CivilizationStage stage{CivilizationStage::Unknown};
    std::string stage_label_key;
    std::vector<CapabilityType> stable_capabilities;
    std::vector<CapabilityType> emerging_capabilities;
    std::vector<CapabilityType> blocked_capabilities;
    std::vector<CapabilityType> next_candidates;
    std::vector<std::string> active_effect_summary_keys;
    std::vector<std::string> risk_warning_keys;
    std::string explanation_key;

    Result<void> validateBasic() const;
};

struct CivilizationTrace {
    EventId trace_id;
    std::vector<std::string> input_summary_keys;
    std::vector<std::string> candidate_steps;
    std::vector<std::string> requirement_steps;
    std::vector<std::string> state_transition_steps;
    std::vector<std::string> effect_steps;
    std::vector<std::string> stage_steps;
    std::vector<std::string> rejected_unsafe_keys;

    Result<void> validateBasic() const;
};

struct CivilizationEventDraft {
    EventId event_id;
    std::string event_type_key;
    TribeId tribe_id;
    std::optional<CapabilityType> capability_type;
    std::optional<CivilizationStage> stage_before;
    std::optional<CivilizationStage> stage_after;
    std::string message_key;
    std::vector<EventId> evidence_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// Security summary DTOs
// ---------------------------------------------------------------------------

struct CivilizationKnowledgeSummary {
    TribeId tribe_id;
    int known_edible_count{0};
    int known_danger_count{0};
    int stable_tribe_knowledge_count{0};
    int conflicted_knowledge_count{0};
    int teachable_knowledge_count{0};
    std::vector<KnowledgeId> knowledge_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationPropagationSummary {
    TribeId tribe_id;
    double coverage{0.0};
    double success_rate{0.0};
    double misunderstanding_rate{0.0};
    std::vector<EventId> teaching_event_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationCoordinationSummary {
    TribeId tribe_id;
    double coordination_score{0.0};
    int stable_coordination_count{0};
    int retreat_success_count{0};
    int resource_defense_success_count{0};
    std::vector<EventId> source_event_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationConflictSummary {
    TribeId tribe_id;
    int intimidation_success_count{0};
    int forced_retreat_low_loss_count{0};
    int truce_success_count{0};
    int standoff_controlled_count{0};
    double open_conflict_pressure{0.0};
    double loss_pressure{0.0};
    std::vector<EventId> source_event_ids;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// Input / Result
// ---------------------------------------------------------------------------

struct CivilizationResolveInput {
    CivilizationState current_state;
    CivilizationKnowledgeSummary knowledge_summary;
    CivilizationPropagationSummary propagation_summary;
    CivilizationCoordinationSummary coordination_summary;
    CivilizationConflictSummary conflict_summary;
    std::vector<CivilizationCapability> capability_definitions;
    Tick input_tick;
    std::vector<std::string> reason_keys;

    Result<void> validateBasic() const;
};

struct CivilizationResolveResult {
    bool ok{false};
    std::optional<pathfinder::foundation::ErrorCode> error;
    CivilizationState updated_state;
    std::vector<CivilizationCapabilityCandidate> candidates;
    std::vector<CivilizationStateChangeDraft> state_changes;
    std::vector<CapabilityEffectDraft> effect_drafts;
    std::vector<CivilizationEventDraft> event_drafts;
    CivilizationProjection projection;
    CivilizationTrace trace;

    Result<void> validateBasic() const;
};

} // namespace pathfinder::civilization
