#pragma once

#include "pathfinder/goal_execution/goal_execution_types.h"
#include "pathfinder/reaction/reaction_rule.h"
#include <memory>
#include <unordered_map>

namespace pathfinder::goal_execution {

struct DriverCheckResult {
    bool ok{true};
    std::vector<InternalBlocker> blockers;
};

class GoalStackManager {
public:
    explicit GoalStackManager(uint32_t max_depth = 8);

    pathfinder::foundation::Result<void> pushGoal(ExecutionContext& context, GoalFrame frame) const;
    pathfinder::foundation::Result<void> pauseFrame(ExecutionContext& context, const std::string& frame_id, const std::string& reason_key, uint64_t tick) const;
    pathfinder::foundation::Result<void> resumeFrame(ExecutionContext& context, const std::string& frame_id) const;
    pathfinder::foundation::Result<void> completeFrame(ExecutionContext& context, const std::string& frame_id) const;
    pathfinder::foundation::Result<void> cancelFrame(ExecutionContext& context, const std::string& frame_id) const;
    pathfinder::foundation::Result<GoalFrame> activeFrame(const ExecutionContext& context) const;

private:
    uint32_t max_depth_{8};
};

class ActionDriver {
public:
    virtual ~ActionDriver() = default;
    virtual ActionDriverKind kind() const = 0;
    virtual bool canHandle(const DriverCommand& command) const = 0;
    virtual pathfinder::foundation::Result<DriverCheckResult> checkPreconditions(const DriverTickInput& input) const = 0;
    virtual pathfinder::foundation::Result<DriverTickResult> tick(const DriverTickInput& input) const = 0;
};

class ActionDriverRegistry {
public:
    pathfinder::foundation::Result<void> registerDriver(std::unique_ptr<ActionDriver> driver);
    pathfinder::foundation::Result<const ActionDriver*> find(ActionDriverKind kind, AgentCapabilityTier tier = AgentCapabilityTier::CognitiveAgent) const;
    pathfinder::foundation::Result<void> validateAll() const;
    pathfinder::foundation::Result<void> registerDefaultDrivers();

private:
    std::unordered_map<ActionDriverKind, std::unique_ptr<ActionDriver>> drivers_;
};

class MaterialRequirementEvaluator {
public:
    pathfinder::foundation::Result<MaterialEvaluationResult> evaluate(const MaterialEvaluationInput& input) const;
    pathfinder::foundation::Result<std::vector<InternalBlocker>> toBlockers(const MaterialEvaluationResult& result, const std::string& actor_key, const std::string& driver_state_id) const;
};

class MaterialReservationManager {
public:
    pathfinder::foundation::Result<ResourceReservation> reserve(ExecutionContext& context, const MaterialReserveRequest& request) const;
    pathfinder::foundation::Result<void> release(ExecutionContext& context, const std::string& reservation_id) const;
    pathfinder::foundation::Result<void> releaseByGoal(ExecutionContext& context, const std::string& goal_id) const;
    pathfinder::foundation::Result<int> reservedQuantity(const ExecutionContext& context, const std::string& object_key) const;
};

class ReactionMaterialResolver {
public:
    pathfinder::foundation::Result<std::vector<ReactionMaterialLink>> findProducers(
        const MaterialRequirement& requirement,
        const std::vector<pathfinder::reaction::ObjectReactionRule>& rules) const;

    pathfinder::foundation::Result<std::vector<ReactionMaterialLink>> findProducers(const MaterialRequirement& requirement) const;
};

class InternalBlockerResolver {
public:
    pathfinder::foundation::Result<std::vector<pathfinder::agent_reasoning::AgentGoal>> resolve(const InternalBlockerResolveInput& input) const;
};

class InterruptPriorityEvaluator {
public:
    double score(const ExternalInterruptSignal& signal, const ExecutionContext& context, const GoalFrame* active_frame) const;
};

class InterruptSystem {
public:
    pathfinder::foundation::Result<std::vector<InterruptDecision>> evaluate(const InterruptEvaluationInput& input) const;
};

class ExecutionResumeValidator {
public:
    pathfinder::foundation::Result<ResumeDecision> validate(const ExecutionContext& context, const PausedExecutionContext& paused, const pathfinder::world_interaction::WorldSnapshot& snapshot, uint64_t tick) const;
};

class DriverCommandAdapter {
public:
    pathfinder::foundation::Result<std::vector<pathfinder::world_interaction::WorldInteractionInput>> adapt(const std::vector<DriverCommand>& commands) const;
};

class ExecutionProjectionMapper {
public:
    ExecutionStatusProjection map(const ExecutionResult& result) const;
};

class GoalExecutionSystem {
public:
    GoalExecutionSystem();
    pathfinder::foundation::Result<ExecutionResult> tick(const ExecutionTickRequest& request) const;

private:
    GoalStackManager stack_manager_;
    ActionDriverRegistry registry_;
    InterruptSystem interrupt_system_;
    InternalBlockerResolver blocker_resolver_;
    DriverCommandAdapter command_adapter_;
    ExecutionResumeValidator resume_validator_;
    ExecutionProjectionMapper projection_mapper_;
};

DriverExecutionState driverStateFromPlanStep(const pathfinder::agent_reasoning::AgentPlanStep& step);
GoalFrame goalFrameFromPlan(const pathfinder::agent_reasoning::AgentPlan& plan, uint64_t tick);

} // namespace pathfinder::goal_execution
