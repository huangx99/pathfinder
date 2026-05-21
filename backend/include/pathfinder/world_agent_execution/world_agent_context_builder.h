#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"

namespace pathfinder::world_agent_execution {

class WorldAgentContextBuilder {
public:
    WorldAgentContextBuilder(
        IAgentVisibleWorldQueryPort& visible_world_port,
        IAgentInventoryQueryPort& inventory_port,
        IAgentKnowledgeQueryPort& knowledge_port,
        IWorldAgentExecutionContextStore& context_store);

    pathfinder::foundation::Result<WorldAgentDecisionContext> build(const WorldAgentTickRequest& request);

private:
    IAgentVisibleWorldQueryPort& visible_world_port_;
    IAgentInventoryQueryPort& inventory_port_;
    IAgentKnowledgeQueryPort& knowledge_port_;
    IWorldAgentExecutionContextStore& context_store_;
};

} // namespace pathfinder::world_agent_execution
