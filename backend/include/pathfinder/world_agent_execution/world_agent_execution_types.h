#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/world_inventory/world_inventory_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/goal_execution/goal_execution_types.h"
#include "pathfinder/agent_reasoning/reasoning_types.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <map>

namespace pathfinder::world_agent_execution {

// ============================================================
// 9. Core Enums
// ============================================================

enum class WorldAgentExecutionDecision {
    Unknown,
    NoopNoGoal,
    SelectedGoal,
    PlannedStep,
    IssuedCommand,
    CommandSucceeded,
    CommandBlocked,
    CommandFailed,
    PausedForInterrupt,
    InsertedSubGoal,
    WaitingForKnowledge,
    CompletedGoal,
    TestOnly
};

std::string toString(WorldAgentExecutionDecision decision);
pathfinder::foundation::Result<WorldAgentExecutionDecision> worldAgentExecutionDecisionFromString(const std::string& str);

enum class WorldAgentExecutionFailureKind {
    None,
    InvalidRequest,
    ActorMissing,
    ContextBuildFailed,
    KnowledgeMissing,
    ReasoningFailed,
    PlanEmpty,
    CommandCompileFailed,
    CommandPipelineFailed,
    CommandBlocked,
    RuntimeUnavailable,
    InventoryUnavailable,
    OwnershipInvalid,
    Interrupted,
    MaxDepthExceeded,
    ForbiddenBySafety,
    TestOnly
};

std::string toString(WorldAgentExecutionFailureKind kind);
pathfinder::foundation::Result<WorldAgentExecutionFailureKind> worldAgentExecutionFailureKindFromString(const std::string& str);

enum class WorldAgentGoalSourceKind {
    Unknown,
    InternalNeed,
    PlayerOrder,
    WorldThreat,
    ExternalInterrupt,
    KnowledgeOpportunity,
    QueuedGoal,
    SystemMaintenance,
    TestOnly
};

std::string toString(WorldAgentGoalSourceKind kind);
pathfinder::foundation::Result<WorldAgentGoalSourceKind> worldAgentGoalSourceKindFromString(const std::string& str);

enum class WorldAgentStepCommandKind {
    Unknown,
    Move,
    Gather,
    Chop,
    Mine,
    Dig,
    Pickup,
    Drop,
    Eat,
    Use,
    Craft,
    Teach,
    Attack,
    Flee,
    Wait,
    CastAreaEffect,
    TriggerAreaEvent,
    TestOnly
};

std::string toString(WorldAgentStepCommandKind kind);
pathfinder::foundation::Result<WorldAgentStepCommandKind> worldAgentStepCommandKindFromString(const std::string& str);

// ============================================================
// 10. Visible Refs (P51-specific view into world)
// ============================================================

struct VisibleResourceRef {
    std::string node_id;
    std::string resource_key;
    pathfinder::world_runtime::WorldCellCoord coord;
    int distance = 0;
    int remaining_charges = 1;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> tag_keys;
};

struct VisibleEntityRef {
    std::string entity_id;
    std::string entity_key;
    pathfinder::world_runtime::WorldCellCoord coord;
    int distance = 0;
    std::string owner_ref;
    bool is_actor = false;
    std::vector<std::string> tag_keys;
};

// ============================================================
// 11. Core DTOs
// ============================================================

struct WorldAgentTickRequest {
    std::string request_id;
    std::string actor_key;
    uint64_t tick = 0;
    bool allow_issue_command = true;
    bool allow_replan = true;
    bool allow_hypothesis_knowledge = true;
    bool allow_risk_action = false;
    uint32_t max_subgoal_depth = 8;
    std::vector<std::string> queued_goal_ids;
    std::vector<pathfinder::goal_execution::ExternalInterruptSignal> external_interrupts;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldAgentDecisionContext {
    std::string context_id;
    std::string actor_key;
    pathfinder::world_runtime::WorldActorRuntime actor;
    pathfinder::world_runtime::WorldRuntimeSnapshot runtime_snapshot;
    pathfinder::world_inventory::InventoryRuntimeSnapshot inventory_snapshot;
    std::vector<pathfinder::knowledge::KnowledgeClaim> actor_claims;
    std::vector<VisibleResourceRef> visible_resources;
    std::vector<VisibleEntityRef> visible_entities;
    std::optional<pathfinder::goal_execution::ExecutionContext> existing_execution_context;
    std::vector<pathfinder::goal_execution::ExternalInterruptSignal> external_interrupts;
    uint64_t tick = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldAgentCompiledCommand {
    bool ok = false;
    WorldAgentStepCommandKind step_command_kind = WorldAgentStepCommandKind::Unknown;
    pathfinder::world_command::WorldCommandDto command;
    std::vector<pathfinder::goal_execution::InternalBlocker> blockers;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldAgentTickResult {
    bool ok = false;
    WorldAgentExecutionDecision decision = WorldAgentExecutionDecision::Unknown;
    WorldAgentExecutionFailureKind failure_kind = WorldAgentExecutionFailureKind::None;
    std::string actor_key;
    std::optional<pathfinder::agent_reasoning::AgentGoal> selected_goal;
    std::optional<pathfinder::agent_reasoning::AgentPlan> selected_plan;
    std::optional<pathfinder::agent_reasoning::AgentPlanStep> selected_step;
    std::optional<pathfinder::world_command::WorldCommandDto> issued_command;
    std::optional<pathfinder::world_command::WorldCommandExecutionResult> command_result;
    pathfinder::goal_execution::ExecutionContext execution_context_after;
    std::vector<pathfinder::goal_execution::InternalBlocker> blockers;
    std::vector<pathfinder::goal_execution::InterruptDecision> interrupt_decisions;
    std::vector<pathfinder::world_command::WorldEventDto> events;
    std::vector<pathfinder::world_command::WorldStateDeltaDto> state_deltas;
    std::optional<pathfinder::world_command::WorldProjectionPatchDto> projection_patch;
    std::vector<std::string> reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ============================================================
// 12. Port Interfaces
// ============================================================

class IWorldAgentExecutionContextStore {
public:
    virtual ~IWorldAgentExecutionContextStore() = default;
    virtual pathfinder::foundation::Result<std::optional<pathfinder::goal_execution::ExecutionContext>> findByActor(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<void> saveContext(const pathfinder::goal_execution::ExecutionContext& context) = 0;
    virtual pathfinder::foundation::Result<void> clearActor(const std::string& actor_key) = 0;
};

class IAgentCommandPipelinePort {
public:
    virtual ~IAgentCommandPipelinePort() = default;
    virtual pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> executeAgentCommand(const pathfinder::world_command::WorldCommandDto& command) = 0;
};

class IAgentVisibleWorldQueryPort {
public:
    virtual ~IAgentVisibleWorldQueryPort() = default;
    virtual pathfinder::foundation::Result<pathfinder::world_runtime::WorldActorRuntime> findActor(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<pathfinder::world_runtime::WorldRuntimeSnapshot> visibleSnapshotForActor(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<VisibleResourceRef>> visibleResourcesForActor(const std::string& actor_key) const = 0;
    virtual pathfinder::foundation::Result<std::vector<VisibleEntityRef>> visibleEntitiesForActor(const std::string& actor_key) const = 0;
};

class IAgentInventoryQueryPort {
public:
    virtual ~IAgentInventoryQueryPort() = default;
    virtual pathfinder::foundation::Result<pathfinder::world_inventory::InventoryRuntimeSnapshot> snapshotForActorDecision(const std::string& actor_key) const = 0;
};

class IAgentKnowledgeQueryPort {
public:
    virtual ~IAgentKnowledgeQueryPort() = default;
    virtual pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> claimsForActor(const std::string& actor_key) const = 0;
};

// ============================================================
// 13. Safety / Forbidden Keys
// ============================================================

std::vector<std::string> worldAgentExecutionForbiddenKeys();
bool containsWorldAgentExecutionForbiddenKey(const std::string& text);
bool containsWorldAgentExecutionForbiddenKey(const std::vector<std::string>& values);

} // namespace pathfinder::world_agent_execution
