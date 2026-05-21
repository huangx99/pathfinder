#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/world_agent_context_builder.h"
#include "pathfinder/world_agent_execution/world_agent_goal_selector.h"
#include "pathfinder/world_agent_execution/world_agent_reasoning_bridge.h"
#include "pathfinder/world_agent_execution/agent_action_knowledge_guard.h"
#include "pathfinder/world_agent_execution/agent_plan_command_compiler.h"
#include "pathfinder/world_agent_execution/world_agent_projection_bridge.h"
#include "pathfinder/world_teaching/npc_basic_action_knowledge_gate.h"
#include "pathfinder/agent_reasoning/agent_reasoner.h"

namespace pathfinder::world_agent_execution {

class AgentExecutionCoordinator {
public:
    AgentExecutionCoordinator(
        WorldAgentContextBuilder& context_builder,
        WorldAgentGoalSelector& goal_selector,
        WorldAgentReasoningBridge& reasoning_bridge,
        AgentActionKnowledgeGuard& knowledge_guard,
        AgentPlanCommandCompiler& command_compiler,
        IAgentCommandPipelinePort& pipeline_port,
        IWorldAgentExecutionContextStore& context_store,
        WorldAgentProjectionBridge& projection_bridge);

    pathfinder::foundation::Result<WorldAgentTickResult> tick(const WorldAgentTickRequest& request);

private:
    WorldAgentContextBuilder& context_builder_;
    WorldAgentGoalSelector& goal_selector_;
    WorldAgentReasoningBridge& reasoning_bridge_;
    AgentActionKnowledgeGuard& knowledge_guard_;
    AgentPlanCommandCompiler& command_compiler_;
    IAgentCommandPipelinePort& pipeline_port_;
    IWorldAgentExecutionContextStore& context_store_;
    WorldAgentProjectionBridge& projection_bridge_;

    pathfinder::foundation::Result<WorldAgentTickResult> makeFailureResult(
        const WorldAgentTickRequest& request,
        WorldAgentExecutionFailureKind failure_kind,
        const std::vector<std::string>& reason_keys);
};

} // namespace pathfinder::world_agent_execution
