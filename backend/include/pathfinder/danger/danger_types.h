#pragma once

#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/reaction/reaction_content.h"
#include "pathfinder/reaction/reaction_learning_bridge.h"
#include "pathfinder/reaction/reaction_rule.h"
#include "pathfinder/tribe/tribe_coordination.h"
#include "pathfinder/tribe/tribe_state.h"
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::danger {

enum class DangerSourceKind {
    Unknown,
    Environment,
    ObjectReaction,
    Creature,
    TribeConflict,
    ResourcePressure,
    KnowledgeConflict,
    TestOnly
};

enum class HazardTriggerKind {
    Unknown,
    Explore,
    Approach,
    Touch,
    Eat,
    Use,
    Combine,
    Rest,
    Teach,
    Conflict,
    TestOnly
};

enum class DangerSeverity {
    Unknown,
    None,
    Notice,
    Strain,
    Harm,
    Severe,
    Critical,
    TestOnly
};

enum class DangerDecision {
    Unknown,
    Safe,
    Warned,
    Exposed,
    Avoided,
    Escaped,
    BlockedByCondition,
    NoMatchingHazard,
    Rejected,
    TestOnly
};

enum class FearSignalKind {
    Unknown,
    None,
    Caution,
    Alarm,
    Panic,
    GroupAnxiety,
    TestOnly
};

enum class CountermeasureKind {
    Unknown,
    Knowledge,
    Tool,
    GroupSupport,
    Distance,
    EscapeRoute,
    Civilization,
    TestOnly
};

std::string toString(DangerSourceKind kind);
std::string toString(HazardTriggerKind kind);
std::string toString(DangerSeverity severity);
std::string toString(DangerDecision decision);
std::string toString(FearSignalKind kind);
std::string toString(CountermeasureKind kind);

pathfinder::foundation::Result<DangerSourceKind> dangerSourceKindFromString(const std::string& value);
pathfinder::foundation::Result<HazardTriggerKind> hazardTriggerKindFromString(const std::string& value);
pathfinder::foundation::Result<DangerSeverity> dangerSeverityFromString(const std::string& value);
pathfinder::foundation::Result<DangerDecision> dangerDecisionFromString(const std::string& value);
pathfinder::foundation::Result<FearSignalKind> fearSignalKindFromString(const std::string& value);
pathfinder::foundation::Result<CountermeasureKind> countermeasureKindFromString(const std::string& value);

bool containsDangerForbiddenKey(const std::string& key);
bool containsDangerForbiddenKey(const std::vector<std::string>& keys);
bool validDangerRatio(double value);
double clampDangerRatio(double value);
int severityRank(DangerSeverity severity);
DangerSeverity severityFromRank(int rank);
DangerSeverity reduceSeverity(DangerSeverity severity, double mitigation_score);
FearSignalKind fearKindForSeverity(DangerSeverity severity, bool affects_group = false);

struct ThreatProfile {
    std::string threat_key;
    DangerSourceKind source_kind{DangerSourceKind::Unknown};
    pathfinder::foundation::EntityId source_entity_id;
    pathfinder::foundation::ObjectId source_object_id;
    pathfinder::foundation::ObjectDefinitionId source_definition_id;
    std::string public_name_key;
    double base_pressure{0.0};
    double proximity_pressure{0.0};
    double visibility{1.0};
    double predictability{0.5};
    DangerSeverity base_severity{DangerSeverity::Unknown};
    std::vector<std::string> safe_tag_keys;
    std::vector<std::string> state_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CountermeasureRequirement {
    CountermeasureKind kind{CountermeasureKind::Unknown};
    std::string requirement_key;
    pathfinder::condition::ConditionExpressionRef condition_ref;
    double mitigation_score{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct HazardRule {
    std::string rule_key;
    HazardTriggerKind trigger_kind{HazardTriggerKind::Unknown};
    DangerSourceKind source_kind{DangerSourceKind::Unknown};
    std::vector<pathfinder::condition::ConditionExpressionRef> condition_refs;
    std::vector<CountermeasureRequirement> countermeasure_requirements;
    DangerSeverity severity{DangerSeverity::Unknown};
    double pressure_delta{0.0};
    double fear_delta{0.0};
    double morale_delta{0.0};
    double trust_delta{0.0};
    double split_risk_delta{0.0};
    std::string feedback_key;
    std::string feedback_text;
    std::string knowledge_effect_key;
    std::vector<std::string> safe_tags;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct HazardExposureInput {
    std::string input_key;
    pathfinder::foundation::EntityId actor_id;
    pathfinder::foundation::TribeId tribe_id;
    HazardTriggerKind trigger_kind{HazardTriggerKind::Unknown};
    std::vector<ThreatProfile> threats;
    std::vector<pathfinder::reaction::ReactionRuntimeObject> available_objects;
    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_knowledge_claims;
    std::vector<pathfinder::knowledge::KnowledgeClaim> tribe_shared_claims;
    std::optional<pathfinder::tribe::TribeProjection> tribe_projection;
    std::optional<pathfinder::tribe::CombatCoordinationResult> coordination_projection;
    pathfinder::foundation::Tick tick;
    std::vector<std::string> safe_context_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct InjuryStateDraft {
    pathfinder::foundation::EntityId actor_id;
    DangerSeverity severity{DangerSeverity::Unknown};
    std::string injury_key;
    double pressure_delta{0.0};
    bool blocks_action{false};
    bool requires_help{false};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct FearSignal {
    FearSignalKind kind{FearSignalKind::Unknown};
    double fear_pressure{0.0};
    double confidence{0.0};
    bool visible_to_player{true};
    bool affects_group{false};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DangerFeedbackDraft {
    std::string feedback_key;
    std::string safe_text;
    DangerSeverity visible_severity{DangerSeverity::Unknown};
    std::vector<std::string> warning_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct DangerKnowledgeDraft {
    std::string knowledge_key;
    std::string subject_id;
    std::string action_key;
    std::string effect_key;
    DangerSeverity severity{DangerSeverity::Unknown};
    std::vector<pathfinder::knowledge::KnowledgeCondition> conditions;
    bool shareable{true};
    bool teachable{true};
    bool npc_learnable{true};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TribeDangerPressureDraft {
    pathfinder::foundation::TribeId tribe_id;
    double morale_delta{0.0};
    double trust_delta{0.0};
    double safety_pressure{0.0};
    double resource_pressure{0.0};
    double knowledge_conflict_pressure{0.0};
    double casualty_pressure{0.0};
    double split_risk_hint{0.0};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct HazardTrace {
    std::string trace_key;
    std::vector<std::string> matched_rule_keys;
    std::vector<std::string> blocked_rule_keys;
    std::vector<std::string> applied_countermeasure_keys;
    std::vector<std::string> condition_trace_keys;
    DangerDecision decision{DangerDecision::Unknown};
    DangerSeverity final_severity{DangerSeverity::Unknown};
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct HazardResolutionResult {
    DangerDecision decision{DangerDecision::Unknown};
    DangerSeverity severity{DangerSeverity::Unknown};
    std::vector<InjuryStateDraft> injury_drafts;
    std::vector<FearSignal> fear_signals;
    std::vector<DangerFeedbackDraft> feedbacks;
    std::vector<DangerKnowledgeDraft> knowledge_drafts;
    std::vector<TribeDangerPressureDraft> tribe_pressure_drafts;
    std::vector<pathfinder::reaction::ReactionEventDraft> event_drafts;
    HazardTrace trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct CountermeasureAssessment {
    double mitigation_score{0.0};
    std::vector<std::string> applied_countermeasure_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::danger
