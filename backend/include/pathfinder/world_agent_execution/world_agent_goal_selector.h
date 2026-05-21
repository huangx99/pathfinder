#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"

namespace pathfinder::world_agent_execution {

class WorldAgentGoalSelector {
public:
    WorldAgentGoalSelector();

    struct SelectionResult {
        bool has_goal = false;
        pathfinder::agent_reasoning::AgentGoal goal;
        WorldAgentGoalSourceKind source_kind = WorldAgentGoalSourceKind::Unknown;
        std::vector<std::string> reason_keys;
    };

    pathfinder::foundation::Result<SelectionResult> select(
        const WorldAgentDecisionContext& context,
        const WorldAgentTickRequest& request);

private:
    bool hasActiveGoal(const pathfinder::goal_execution::ExecutionContext& exec_ctx) const;
    pathfinder::foundation::Result<SelectionResult> selectFromInterrupts(
        const WorldAgentDecisionContext& context);
    pathfinder::foundation::Result<SelectionResult> selectFromActiveContext(
        const pathfinder::goal_execution::ExecutionContext& exec_ctx);
    pathfinder::foundation::Result<SelectionResult> selectFromQueuedGoals(
        const WorldAgentTickRequest& request);
    pathfinder::foundation::Result<SelectionResult> selectFromNeeds(
        const WorldAgentDecisionContext& context);
};

} // namespace pathfinder::world_agent_execution
