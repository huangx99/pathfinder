#pragma once

#include "pathfinder/tribe/tribe_coordination.h"
#include "pathfinder/tribe/tribe_state.h"
#include "pathfinder/tribe/tribe_types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::tribe {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::EventId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::Tick;
using pathfinder::foundation::TribeId;

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

enum class HostilityState {
    Unknown,
    Neutral,
    Wary,
    Threatened,
    Hostile,
    OpenConflict,
    Retreating,
    Truce,
    TestOnly
};

std::string toString(HostilityState state);
pathfinder::foundation::Result<HostilityState> hostilityStateFromString(const std::string& str);

enum class ConflictIntentKind {
    Unknown,
    Avoid,
    Intimidate,
    DefendTerritory,
    ContestResource,
    Raid,
    Pursue,
    Retreat,
    NegotiateTruce,
    TestOnly
};

std::string toString(ConflictIntentKind kind);
pathfinder::foundation::Result<ConflictIntentKind> conflictIntentKindFromString(const std::string& str);

enum class ConflictStanceKind {
    Unknown,
    Passive,
    Defensive,
    Assertive,
    Aggressive,
    Desperate,
    TestOnly
};

std::string toString(ConflictStanceKind kind);
pathfinder::foundation::Result<ConflictStanceKind> conflictStanceKindFromString(const std::string& str);

enum class ConflictOutcomeKind {
    Unknown,
    NoContact,
    Avoided,
    Intimidated,
    Standoff,
    ForcedRetreat,
    MinorLossPressure,
    MajorLossPressure,
    TruceOffered,
    TestOnly
};

std::string toString(ConflictOutcomeKind kind);
pathfinder::foundation::Result<ConflictOutcomeKind> conflictOutcomeKindFromString(const std::string& str);

enum class ConflictEventKind {
    Unknown,
    RelationChanged,
    HostilityChanged,
    IntimidationApplied,
    RetreatTriggered,
    ResourceContested,
    ConflictPressureChanged,
    TruceProposed,
    TestOnly
};

std::string toString(ConflictEventKind kind);
pathfinder::foundation::Result<ConflictEventKind> conflictEventKindFromString(const std::string& str);

// ---------------------------------------------------------------------------
// Forbidden key helpers
// ---------------------------------------------------------------------------

bool containsConflictForbiddenKey(const std::string& key);
bool containsConflictForbiddenKey(const std::vector<std::string>& keys);

// ---------------------------------------------------------------------------
// DTOs
// ---------------------------------------------------------------------------

struct TribeRelation {
    TribeId source_tribe_id;
    TribeId target_tribe_id;
    HostilityState hostility_state{HostilityState::Neutral};
    double trust_score{0.0};
    double fear_pressure{0.0};
    double grievance_pressure{0.0};
    double resource_tension{0.0};
    std::optional<Tick> last_conflict_tick;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeRelationDraft {
    TribeId source_tribe_id;
    TribeId target_tribe_id;
    HostilityState old_hostility_state{HostilityState::Unknown};
    HostilityState new_hostility_state{HostilityState::Unknown};
    double trust_delta{0.0};
    double fear_pressure_delta{0.0};
    double grievance_pressure_delta{0.0};
    double resource_tension_delta{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictPressureSummary {
    std::string conflict_key;
    double resource_pressure{0.0};
    double territory_pressure{0.0};
    double fear_pressure{0.0};
    double misunderstanding_pressure{0.0};
    double survival_pressure{0.0};
    double visibility_pressure{0.0};
    std::vector<KnowledgeId> known_public_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictParticipantTribe {
    TribeId tribe_id;
    std::string role_in_conflict;
    int member_count_summary{0};
    int active_population_summary{0};
    double morale_summary{0.0};
    double trust_summary{0.0};
    double split_risk_summary{0.0};
    double safety_pressure_summary{0.0};
    double loss_pressure_summary{0.0};
    CombatCoordinationResult coordination_result;
    CombatCoordinationStateDraft coordination_state_draft;
    std::vector<KnowledgeId> public_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictIntent {
    TribeId tribe_id;
    ConflictIntentKind intent_kind{ConflictIntentKind::Unknown};
    ConflictStanceKind stance_kind{ConflictStanceKind::Unknown};
    double confidence{0.0};
    double pressure_score{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictStateDraft {
    TribeId tribe_id;
    double morale_delta{0.0};
    double trust_delta{0.0};
    double safety_pressure_delta{0.0};
    double loss_pressure_delta{0.0};
    double split_risk_delta{0.0};
    double retreat_pressure_delta{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictEvent {
    EventId event_id;
    ConflictEventKind event_kind{ConflictEventKind::Unknown};
    TribeId source_tribe_id;
    TribeId target_tribe_id;
    std::optional<HostilityState> hostility_before;
    std::optional<HostilityState> hostility_after;
    std::optional<ConflictOutcomeKind> outcome_kind;
    std::string public_message_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictProjection {
    TribeId source_tribe_id;
    TribeId target_tribe_id;
    HostilityState visible_hostility_state{HostilityState::Unknown};
    ConflictOutcomeKind visible_outcome{ConflictOutcomeKind::Unknown};
    std::string visible_pressure_level;
    std::string public_summary_key;
    std::vector<std::string> public_reason_keys;
    bool relation_changed{false};
    bool state_pressure_changed{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictTrace {
    EventId trace_id;
    std::vector<std::string> input_summary_keys;
    ConflictIntent source_intent;
    ConflictIntent target_intent;
    double source_coordination_score{0.0};
    double target_coordination_score{0.0};
    std::vector<std::string> score_breakdown;
    std::vector<std::string> outcome_reason_keys;
    std::vector<std::string> relation_transition_reason_keys;
    std::vector<std::string> rejected_unsafe_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictResolutionOptions {
    bool allow_test_only{false};
    double intimidation_threshold{0.65};
    double retreat_threshold{0.35};
    double standoff_gap_threshold{0.15};
    double hostility_escalation_threshold{0.70};
    double truce_pressure_threshold{0.25};
    double max_pressure_delta_per_tick{0.25};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictResolutionInput {
    ConflictParticipantTribe source;
    ConflictParticipantTribe target;
    TribeRelation source_relation_to_target;
    std::optional<TribeRelation> target_relation_to_source;
    ConflictPressureSummary pressure_summary;
    ConflictResolutionOptions options;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConflictResolutionResult {
    bool ok{false};
    std::optional<pathfinder::foundation::ErrorCode> error;
    ConflictOutcomeKind outcome_kind{ConflictOutcomeKind::Unknown};
    ConflictStateDraft source_state_draft;
    ConflictStateDraft target_state_draft;
    TribeRelationDraft source_relation_draft;
    std::optional<TribeRelationDraft> target_relation_draft;
    std::vector<ConflictEvent> events;
    ConflictProjection projection;
    ConflictTrace trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// Builders and resolver
// ---------------------------------------------------------------------------

class ConflictProjectionBuilder {
public:
    pathfinder::foundation::Result<ConflictProjection> build(
        const ConflictResolutionResult& result) const;
};

class ConflictStateDraftBuilder {
public:
    pathfinder::foundation::Result<ConflictStateDraft> buildForTribe(
        const ConflictParticipantTribe& participant,
        ConflictOutcomeKind outcome,
        const ConflictIntent& own_intent,
        const ConflictIntent& opponent_intent) const;
};

class TribeRelationTransitionPolicy {
public:
    pathfinder::foundation::Result<TribeRelationDraft> transition(
        const TribeRelation& current,
        ConflictOutcomeKind outcome,
        const ConflictPressureSummary& pressure) const;
};

class GroupCombatResolver {
public:
    pathfinder::foundation::Result<ConflictResolutionResult> resolve(
        const ConflictResolutionInput& input) const;
};

} // namespace pathfinder::tribe
