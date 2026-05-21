#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/agent_reasoning/agent_reasoner.h"

namespace pathfinder::world_agent_execution {

class WorldAgentReasoningBridge {
public:
    explicit WorldAgentReasoningBridge(const pathfinder::agent_reasoning::AgentReasoner& reasoner);

    struct ReasoningResult {
        bool ok = false;
        std::optional<pathfinder::agent_reasoning::AgentPlan> plan;
        std::vector<pathfinder::goal_execution::InternalBlocker> blockers;
        std::vector<std::string> reason_keys;
    };

    pathfinder::foundation::Result<ReasoningResult> reasonForGoal(
        const WorldAgentDecisionContext& context,
        const pathfinder::agent_reasoning::AgentGoal& goal);

private:
    const pathfinder::agent_reasoning::AgentReasoner& reasoner_;
};

} // namespace pathfinder::world_agent_execution
