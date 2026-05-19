#pragma once

#include "pathfinder/agent_reasoning/effect_semantics.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/reaction/reaction_rule.h"
#include "pathfinder/world_interaction/world_types.h"
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

pathfinder::foundation::Result<EffectExecutionOpKind> effectExecutionOpKindFromString(const std::string& value);
pathfinder::foundation::Result<EffectExecutionTargetKind> effectExecutionTargetKindFromString(const std::string& value);
pathfinder::foundation::Result<ExecutionValueSourceKind> executionValueSourceKindFromString(const std::string& value);
pathfinder::foundation::Result<ExecutionFailureKind> executionFailureKindFromString(const std::string& value);
pathfinder::foundation::Result<AgentStepExecutionMode> agentStepExecutionModeFromString(const std::string& value);

struct EffectExecutionOperation {
    std::string operation_id;
    EffectExecutionOpKind op_kind{EffectExecutionOpKind::Unknown};
    EffectExecutionTargetKind target_kind{EffectExecutionTargetKind::Unknown};
    ExecutionValueSourceKind key_source{ExecutionValueSourceKind::Constant};
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
