#pragma once

#include "pathfinder/tribe/tribe_state.h"
#include "pathfinder/tribe/tribe_types.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::tribe {

using pathfinder::foundation::EntityId;
using pathfinder::foundation::KnowledgeId;
using pathfinder::foundation::Tick;
using pathfinder::foundation::TribeId;

enum class GroupIntentKind {
    Unknown,
    HoldTogether,
    AssistMember,
    DefendGroup,
    FocusThreat,
    ScreenRetreat,
    EscortVulnerable,
    AvoidEngagement,
    TestOnly
};

std::string toString(GroupIntentKind kind);
pathfinder::foundation::Result<GroupIntentKind> groupIntentKindFromString(const std::string& str);

enum class AssistActionKind {
    Unknown,
    Warn,
    GuidePosition,
    ShareTool,
    PullBack,
    CoverApproach,
    StabilizeMorale,
    TestOnly
};

std::string toString(AssistActionKind kind);
pathfinder::foundation::Result<AssistActionKind> assistActionKindFromString(const std::string& str);

enum class DefendActionKind {
    Unknown,
    GuardMember,
    GuardResource,
    BlockThreat,
    FormPerimeter,
    HoldLine,
    ScreenEscape,
    TestOnly
};

std::string toString(DefendActionKind kind);
pathfinder::foundation::Result<DefendActionKind> defendActionKindFromString(const std::string& str);

enum class PackTacticKind {
    Unknown,
    None,
    PairSupport,
    GuardedGather,
    FocusPressure,
    RotatingDefense,
    EncircleThreat,
    RetreatScreen,
    TestOnly
};

std::string toString(PackTacticKind kind);
pathfinder::foundation::Result<PackTacticKind> packTacticKindFromString(const std::string& str);

enum class CoordinationQuality {
    Unknown,
    Failed,
    Weak,
    Stable,
    Strong,
    TestOnly
};

std::string toString(CoordinationQuality quality);
pathfinder::foundation::Result<CoordinationQuality> coordinationQualityFromString(const std::string& str);
CoordinationQuality qualityForScore(double score);

bool containsCoordinationForbiddenKey(const std::string& key);
bool containsCoordinationForbiddenKey(const std::vector<std::string>& keys);

struct ThreatPressureSummary {
    EntityId threat_id;
    std::string threat_key;
    double threat_pressure{0.0};
    double proximity_pressure{0.0};
    double target_pressure{0.0};
    std::vector<KnowledgeId> known_counter_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatParticipant {
    EntityId member_id;
    TribeMemberRole role{TribeMemberRole::Unknown};
    bool available{true};
    double trust{0.5};
    double morale{0.5};
    double fatigue_pressure{0.0};
    double injury_pressure{0.0};
    std::vector<KnowledgeId> known_knowledge_ids;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct GroupIntent {
    GroupIntentKind kind{GroupIntentKind::Unknown};
    double priority{0.0};
    std::optional<EntityId> target_threat_id;
    std::optional<EntityId> target_member_id;
    std::vector<EntityId> participant_ids;
    std::string summary_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AssistAction {
    AssistActionKind kind{AssistActionKind::Unknown};
    EntityId actor_member_id;
    EntityId target_member_id;
    double support_score{0.0};
    double risk_reduction{0.0};
    double trust_delta_hint{0.0};
    double morale_delta_hint{0.0};
    std::string summary_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DefendAction {
    DefendActionKind kind{DefendActionKind::Unknown};
    EntityId actor_member_id;
    std::optional<EntityId> protected_member_id;
    std::optional<std::string> protected_key;
    std::optional<EntityId> threat_id;
    double defense_score{0.0};
    double risk_reduction{0.0};
    std::string summary_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct PackTactic {
    PackTacticKind kind{PackTacticKind::Unknown};
    CoordinationQuality quality{CoordinationQuality::Unknown};
    std::vector<EntityId> participant_ids;
    std::optional<EntityId> focus_threat_id;
    double coordination_score{0.0};
    double risk_reduction{0.0};
    double morale_delta_hint{0.0};
    double trust_delta_hint{0.0};
    std::string summary_key;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct MemberCoordinationSummary {
    EntityId member_id;
    bool participated{false};
    int assist_count{0};
    int defend_count{0};
    int received_assist_count{0};
    double coordination_score{0.0};
    double morale_delta_hint{0.0};
    double trust_delta_hint{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationInput {
    TribeId tribe_id;
    Tick input_tick;
    TribeState tribe_state;
    std::vector<ThreatPressureSummary> threats;
    std::optional<GroupIntentKind> requested_intent;
    double resource_pressure{0.0};
    double safety_pressure{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationOptions {
    double morale_weight{0.25};
    double trust_weight{0.25};
    double role_weight{0.25};
    double threat_weight{0.15};
    double split_risk_penalty_weight{0.10};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationStateDraft {
    TribeId tribe_id;
    Tick input_tick;
    double morale_delta{0.0};
    double trust_delta{0.0};
    double safety_pressure{0.0};
    double casualty_pressure{0.0};
    double knowledge_conflict_pressure{0.0};
    std::vector<MemberCoordinationSummary> member_summaries;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationProjection {
    TribeId tribe_id;
    std::string intent_summary;
    std::string tactic_summary;
    std::string participant_summary;
    std::string quality_summary;
    std::string risk_summary;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationTrace {
    std::string trace_key;
    Tick input_tick;
    std::vector<std::string> steps;
    std::vector<std::string> matched_rule_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CombatCoordinationResult {
    std::string decision;
    GroupIntent intent;
    std::vector<AssistAction> assist_actions;
    std::vector<DefendAction> defend_actions;
    PackTactic pack_tactic;
    std::vector<MemberCoordinationSummary> member_summaries;
    CombatCoordinationStateDraft state_draft;
    CombatCoordinationProjection projection;
    CombatCoordinationTrace trace;
    std::vector<std::string> warning_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class CombatCoordinationProjectionBuilder {
public:
    pathfinder::foundation::Result<CombatCoordinationProjection> build(
        const CombatCoordinationResult& result) const;
};

class CombatCoordinationResolver {
public:
    pathfinder::foundation::Result<CombatCoordinationResult> resolve(
        const CombatCoordinationInput& input,
        const CombatCoordinationOptions& options = {}) const;
};

} // namespace pathfinder::tribe
