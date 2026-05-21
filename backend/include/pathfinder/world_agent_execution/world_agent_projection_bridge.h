#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"

namespace pathfinder::world_agent_execution {

class WorldAgentProjectionBridge {
public:
    WorldAgentProjectionBridge();

    struct AgentProjection {
        std::string actor_key;
        std::string current_goal_summary;
        std::string current_step_summary;
        std::string decision;
        std::string failure_kind;
        std::string issued_command_kind;
        std::vector<std::string> blocker_summaries;
        std::vector<std::string> interrupt_summaries;
        std::vector<std::string> trace_keys;
    };

    AgentProjection build(const WorldAgentTickResult& tick_result);
};

} // namespace pathfinder::world_agent_execution
