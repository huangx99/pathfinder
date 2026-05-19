#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/world_interaction/world_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::agent_reasoning {

enum class AgentGoalKind {
    Unknown,
    ReduceHunger,
    RestoreHealth,
    ReduceCold,
    IncreaseWarmth,
    IncreaseShelterCapacity,
    ReduceThreat,
    AcquireObject,
    RestoreToolState,
    MaintainFire,
    ProtectDependent,
    TestOnly
};

enum class EffectSemanticKind {
    Unknown,
    NeedDelta,
    ConditionDelta,
    ThreatDelta,
    ObjectQuantityDelta,
    ObjectStateDelta,
    CapabilityDelta,
    KnowledgeDelta,
    LocationSafetyDelta,
    TimeCost,
    RiskDelta,
    TestOnly
};

enum class PlanStepKind {
    Unknown,
    DirectAction,
    PrepareObject,
    RestoreTool,
    BuildStructure,
    TeachKnowledge,
    WaitForCondition,
    Abort,
    TestOnly
};

enum class PlanSelectionReason {
    Unknown,
    HighestUtility,
    ShortestChain,
    LowestRisk,
    OnlyExecutable,
    EmergencyOverride,
    InsufficientKnowledge,
    BlockedByResource,
    BlockedByCondition,
    CycleDetected,
    SearchLimitReached,
    TestOnly
};

enum class KnowledgeUsability {
    Unknown,
    Usable,
    Tentative,
    Conflicted,
    Dangerous,
    UnknownEffect,
    NotOwnedByAgent,
    TestOnly
};

std::string toString(AgentGoalKind kind);
std::string toString(EffectSemanticKind kind);
std::string toString(PlanStepKind kind);
std::string toString(PlanSelectionReason reason);
std::string toString(KnowledgeUsability usability);

struct SemanticDelta {
    std::string domain;
    std::string key;
    std::string op{"add"};
    double value{0.0};
    std::string beneficial_when;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct EffectSemantics {
    std::string effect_key;
    std::string display_zh_cn;
    EffectSemanticKind semantic_kind{EffectSemanticKind::Unknown};
    std::string target_scope;
    std::vector<AgentGoalKind> goal_affinities;
    std::vector<SemanticDelta> state_deltas;
    std::optional<std::string> required_target_kind;
    double risk_score{0.0};
    uint32_t time_cost{1};
    pathfinder::knowledge::KnowledgeStatus confidence_floor{pathfinder::knowledge::KnowledgeStatus::Active};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentNeedState {
    std::string actor_key;
    double hunger{0.0};
    double cold{0.0};
    double health{100.0};
    double fear{0.0};
    double fatigue{0.0};
    double dependent_pressure{0.0};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentGoal {
    std::string goal_id;
    std::string actor_key;
    AgentGoalKind kind{AgentGoalKind::Unknown};
    std::optional<std::string> target_key;
    double urgency{0.0};
    double desired_delta{0.0};
    std::vector<std::string> source_keys;
    std::optional<uint64_t> expires_after_ticks;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct PlanPrecondition {
    std::string condition_id;
    std::string condition_expression;
    std::string missing_domain;
    std::string missing_key;
    std::string required_value;
    bool can_be_planned{false};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ActionCandidate {
    std::string candidate_id;
    std::string actor_key;
    std::string source_knowledge_id;
    std::string object_key;
    std::optional<std::string> target_key;
    std::string action_key;
    std::string effect_key;
    EffectSemantics semantics;
    bool is_directly_executable{false};
    std::vector<PlanPrecondition> blocking_conditions;
    KnowledgeUsability knowledge_usability{KnowledgeUsability::Unknown};
    double knowledge_confidence{0.0};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentPlanStep {
    std::string step_id;
    PlanStepKind kind{PlanStepKind::Unknown};
    std::string actor_key;
    std::string object_key;
    std::optional<std::string> target_key;
    std::string action_key;
    std::string effect_key;
    std::vector<EffectSemantics> expected_semantics;
    std::vector<PlanPrecondition> preconditions;
    uint32_t estimated_ticks{1};
    double risk_score{0.0};
    std::string explanation_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentPlan {
    std::string plan_id;
    std::string actor_key;
    AgentGoal goal;
    std::vector<AgentPlanStep> steps;
    uint32_t total_estimated_ticks{0};
    double total_risk_score{0.0};
    double utility_score{0.0};
    PlanSelectionReason selection_reason{PlanSelectionReason::Unknown};
    bool blocked{false};
    std::optional<std::string> blocking_reason_zh_cn;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReasoningOptions {
    uint32_t max_depth{4};
    uint32_t max_plan_steps{6};
    uint32_t max_candidates_per_goal{8};
    uint32_t max_total_expansions{64};
    bool allow_tentative_knowledge{false};
    bool emergency_allows_tentative{true};
    double min_confidence_score{0.5};
    bool enable_trace{true};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReasoningRequest {
    std::string request_id;
    std::string actor_key;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    std::vector<pathfinder::knowledge::KnowledgeClaim> agent_knowledge;
    AgentNeedState need_state;
    ReasoningOptions options;
    std::string trigger_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReasoningTrace {
    std::string trace_id;
    uint32_t goal_count{0};
    uint32_t candidate_count{0};
    uint32_t expanded_count{0};
    bool cycle_detected{false};
    bool limit_reached{false};
    std::optional<std::string> selected_plan_id;
    std::vector<std::string> public_reason_keys;
    std::vector<std::string> debug_reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ReasoningResult {
    bool ok{false};
    std::optional<AgentPlan> selected_plan;
    std::vector<AgentPlan> rejected_plans;
    std::vector<AgentGoal> goals;
    ReasoningTrace trace;
    std::string safe_explanation_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct GoalGenerationInput {
    AgentNeedState need_state;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
};

struct CandidateBuildInput {
    std::string actor_key;
    AgentGoal goal;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    std::vector<pathfinder::knowledge::KnowledgeClaim> agent_knowledge;
    ReasoningOptions options;
};

struct PreconditionResolveInput {
    AgentGoal goal;
    ActionCandidate candidate;
    std::vector<ActionCandidate> all_candidates;
    pathfinder::world_interaction::WorldSnapshot world_snapshot;
    ReasoningOptions options;
};

struct PlanScoreInput {
    AgentGoal goal;
    AgentPlan plan;
    AgentNeedState need_state;
};

struct PlanScore {
    double utility_score{0.0};
    PlanSelectionReason reason{PlanSelectionReason::HighestUtility};
    std::string explanation_zh_cn;
};

struct ReasoningProjection {
    std::string current_goal_zh_cn;
    std::string next_step_zh_cn;
    std::string blocked_reason_zh_cn;
    std::vector<std::string> public_reason_lines_zh_cn;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent_reasoning
