#pragma once

#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/reaction/reaction_rule.h"
#include "pathfinder/world_interaction/world_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pathfinder::agent_reasoning {

enum class EffectExecutionOpKind {
    Unknown,
    ConsumeObjectQuantity,
    AddObjectQuantity,
    SetObjectQuantity,
    AddObjectStateNumber,
    SetObjectStateNumber,
    AddObjectTag,
    RemoveObjectTag,
    ChangeActorNeed,
    ChangeThreatLevel,
    ResolveThreat,
    EmitWorldEvent,
    QueueFollowupGoal,
    ApplyStatusEffect,
    RemoveStatusEffect,
    ChangeRelationship,
    CreateAgreement,
    BreakAgreement,
    ChangeReputation,
    UnlockCapability,
    ChangePopulation,
    ChangeSystemPressure,
    ChangeWorldRule,
    PropagateKnowledge,
    CorruptKnowledge,
    ChangeKnowledgeTrust,
    ScheduleDelayedEffect,
    StartPeriodicEffect,
    StopPeriodicEffect,
    TestOnly
};

enum class EffectExecutionTargetKind {
    Unknown,
    ActorSelf,
    ActorTarget,
    ObjectSelf,
    ObjectTarget,
    Location,
    ThreatTarget,
    GroupOrTribe,
    RuntimeKey,
    InventoryScope,
    Region,
    WorldSystem,
    Faction,
    Civilization,
    Megastructure,
    KnowledgeGraph,
    Agreement,
    Timeline,
    TestOnly
};

enum class WorldScopeKind {
    Unknown,
    Actor,
    Inventory,
    Object,
    Location,
    Region,
    Threat,
    GroupOrTribe,
    Faction,
    Civilization,
    WorldSystem,
    Megastructure,
    KnowledgeGraph,
    Agreement,
    Timeline,
    TestOnly
};

enum class EffectOperationGroupKind {
    Unknown,
    Primary,
    Cost,
    SideEffect,
    Risk,
    Trigger,
    Knowledge,
    Social,
    Systemic,
    Scheduled,
    TestOnly
};

enum class OperationFailurePolicy {
    Unknown,
    FailSpec,
    SkipOperation,
    ContinueWithWarning,
    QueueForRetry,
    TestOnly
};

enum class TemporalEffectKind {
    Unknown,
    Instant,
    Delayed,
    Duration,
    Periodic,
    UntilConditionMet,
    TestOnly
};

enum class TargetSelectionKind {
    Unknown,
    ExplicitRequest,
    ActorSelf,
    NearestMatching,
    AllInScope,
    RandomInScope,
    HighestThreat,
    LowestNeed,
    LinkedKnowledgeTarget,
    TestOnly
};

enum class ExecutionValueSourceKind {
    Unknown,
    Constant,
    RequestObjectKey,
    RequestTargetKey,
    RequestActorKey,
    EffectOutputKey,
    RuntimeExpression,
    TestOnly
};

enum class ExecutionFailureKind {
    Unknown,
    SpecNotFound,
    ConditionNotMet,
    MissingObject,
    MissingTarget,
    InsufficientQuantity,
    StateNotMutable,
    ForbiddenBySafety,
    InvalidOperation,
    ExpressionFailed,
    PartialExecutionRejected,
    TestOnly
};

enum class AgentStepExecutionMode {
    Unknown,
    DryRun,
    ExecuteOneStep,
    ExecuteUntilBlocked,
    ExecuteUntilGoalResolved,
    TestOnly
};

std::string toString(EffectExecutionOpKind kind);
std::string toString(EffectExecutionTargetKind kind);
std::string toString(ExecutionValueSourceKind kind);
std::string toString(ExecutionFailureKind kind);
std::string toString(AgentStepExecutionMode mode);
std::string toString(WorldScopeKind kind);
std::string toString(EffectOperationGroupKind kind);
std::string toString(OperationFailurePolicy policy);
std::string toString(TemporalEffectKind kind);
std::string toString(TargetSelectionKind kind);

pathfinder::foundation::Result<EffectExecutionOpKind> effectExecutionOpKindFromString(const std::string& value);
pathfinder::foundation::Result<EffectExecutionTargetKind> effectExecutionTargetKindFromString(const std::string& value);
pathfinder::foundation::Result<ExecutionValueSourceKind> executionValueSourceKindFromString(const std::string& value);
pathfinder::foundation::Result<ExecutionFailureKind> executionFailureKindFromString(const std::string& value);
pathfinder::foundation::Result<AgentStepExecutionMode> agentStepExecutionModeFromString(const std::string& value);
pathfinder::foundation::Result<WorldScopeKind> worldScopeKindFromString(const std::string& value);
pathfinder::foundation::Result<EffectOperationGroupKind> effectOperationGroupKindFromString(const std::string& value);
pathfinder::foundation::Result<OperationFailurePolicy> operationFailurePolicyFromString(const std::string& value);
pathfinder::foundation::Result<TemporalEffectKind> temporalEffectKindFromString(const std::string& value);
pathfinder::foundation::Result<TargetSelectionKind> targetSelectionKindFromString(const std::string& value);

struct WorldScopeRef {
    WorldScopeKind kind{WorldScopeKind::Unknown};
    std::string scope_key;
    std::string parent_scope_key;
    std::vector<std::string> safe_tags;

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TemporalEffectPolicy {
    TemporalEffectKind kind{TemporalEffectKind::Instant};
    uint64_t delay_ticks{0};
    uint64_t duration_ticks{0};
    uint64_t period_ticks{0};
    std::string until_condition_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct TargetSelectionSpec {
    TargetSelectionKind kind{TargetSelectionKind::ExplicitRequest};
    WorldScopeRef scope;
    std::string explicit_target_key;
    std::vector<std::string> required_tags;
    uint32_t max_targets{1};

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgeEffectPayload {
    std::string knowledge_key;
    std::string subject_key;
    std::string relation_key;
    double trust_delta{0.0};
    std::vector<std::string> propagation_tags;

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct RelationshipEffectPayload {
    std::string source_actor_key;
    std::string target_actor_key;
    std::string relationship_key;
    double relationship_delta{0.0};
    double reputation_delta{0.0};
    std::string agreement_key;

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldRuleEffectPayload {
    std::string rule_key;
    std::string system_key;
    double pressure_delta{0.0};
    std::string capability_key;
    std::string safe_summary_zh_cn;

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct RandomExecutionPolicy {
    bool enabled{false};
    std::string random_stream_key;
    uint32_t seed_salt{0};
    double chance{1.0};
    std::vector<std::string> outcome_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ScheduledEffectRef {
    std::string schedule_id;
    std::string effect_key;
    WorldScopeRef scope;
    uint64_t due_tick{0};
    uint64_t repeat_period_ticks{0};

    bool empty() const;
    pathfinder::foundation::Result<void> validateBasic() const;
};

struct EffectExecutionOperation {
    std::string operation_id;
    EffectExecutionOpKind op_kind{EffectExecutionOpKind::Unknown};
    EffectExecutionTargetKind target_kind{EffectExecutionTargetKind::Unknown};
    EffectOperationGroupKind group_kind{EffectOperationGroupKind::Primary};
    OperationFailurePolicy failure_policy{OperationFailurePolicy::FailSpec};
    ExecutionValueSourceKind key_source{ExecutionValueSourceKind::Constant};
    WorldScopeRef world_scope;
    TargetSelectionSpec target_selection;
    TemporalEffectPolicy temporal_policy;
    KnowledgeEffectPayload knowledge_payload;
    RelationshipEffectPayload relationship_payload;
    WorldRuleEffectPayload world_rule_payload;
    RandomExecutionPolicy random_policy;
    ScheduledEffectRef scheduled_effect;
    std::string runtime_key;
    std::string effect_output_key;
    std::string state_key;
    double number_value{0.0};
    int quantity_value{0};
    std::string safe_summary_zh_cn;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct EffectExecutionSpec {
    std::string spec_id;
    std::string effect_key;
    std::vector<pathfinder::condition::ConditionExpressionRef> condition_refs;
    std::vector<EffectExecutionOperation> operations;
    bool atomic{true};
    bool allow_agent_use{true};
    bool allow_player_use{true};
    WorldScopeRef default_scope;
    TemporalEffectPolicy temporal_policy;
    RandomExecutionPolicy random_policy;
    std::vector<ScheduledEffectRef> scheduled_effects;
    std::vector<std::string> unsupported_features;
    std::string safe_summary_zh_cn;
    std::string source_config_id{"builtin.p41"};
    std::string version{"1"};
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldExecutionRequest {
    std::string request_id;
    std::string actor_key;
    std::string effect_key;
    std::string object_key;
    std::string target_object_key;
    std::string target_threat_key;
    bool dry_run{false};
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldExecutionResult {
    bool ok{false};
    bool executed{false};
    ExecutionFailureKind failure_kind{ExecutionFailureKind::Unknown};
    std::vector<pathfinder::world_interaction::WorldChange> changes;
    std::string safe_summary_zh_cn;
    std::vector<std::string> trace_keys;
    std::string spec_id;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentStepExecutionRequest {
    std::string request_id;
    std::string actor_key;
    std::string plan_id;
    AgentPlanStep step;
    AgentStepExecutionMode mode{AgentStepExecutionMode::Unknown};
    std::string source_knowledge_id;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AgentStepExecutionResult {
    bool ok{false};
    bool executed{false};
    WorldExecutionResult world_result;
    std::string blocked_reason_zh_cn;
    bool should_replan{false};
    std::string consumed_step_id;
    std::vector<std::string> trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class EffectExecutionSpecRegistry {
public:
    EffectExecutionSpecRegistry();
    explicit EffectExecutionSpecRegistry(bool load_builtins);

    pathfinder::foundation::Result<void> registerSpec(const EffectExecutionSpec& spec);
    pathfinder::foundation::Result<EffectExecutionSpec> findByEffectKey(const std::string& effect_key) const;
    pathfinder::foundation::Result<std::vector<EffectExecutionSpec>> allSpecs() const;
    pathfinder::foundation::Result<void> validateAgainst(const EffectSemanticsRegistry& semantics) const;

private:
    std::unordered_map<std::string, EffectExecutionSpec> specs_by_effect_key_;
};

class ExecutionConditionResolver {
public:
    pathfinder::foundation::Result<std::vector<PlanPrecondition>> preconditionsForEffect(
        const std::string& effect_key,
        const EffectExecutionSpecRegistry& execution_specs,
        const std::vector<pathfinder::reaction::ObjectReactionRule>& reaction_rules) const;
};

class WorldEffectExecutor {
public:
    pathfinder::foundation::Result<WorldExecutionResult> execute(
        const pathfinder::world_interaction::WorldSnapshot& snapshot,
        const WorldExecutionRequest& request,
        const EffectExecutionSpecRegistry& specs,
        const pathfinder::condition::ConditionExpressionEvaluator& evaluator) const;
};

class AgentPlanStepExecutor {
public:
    pathfinder::foundation::Result<AgentStepExecutionResult> executeStep(
        const pathfinder::world_interaction::WorldSnapshot& snapshot,
        const AgentStepExecutionRequest& request,
        const EffectExecutionSpecRegistry& specs) const;
};

class AgentExecutionCoordinator {
public:
    pathfinder::foundation::Result<pathfinder::world_interaction::AgentAutonomyResult> executePlan(
        const pathfinder::world_interaction::WorldSnapshot& snapshot,
        const AgentPlan& plan,
        AgentStepExecutionMode mode,
        const EffectExecutionSpecRegistry& specs) const;
};

std::vector<EffectExecutionSpec> builtInEffectExecutionSpecs();

} // namespace pathfinder::agent_reasoning
