#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"

namespace pathfinder::world_agent_execution {

class AgentPlanCommandCompiler {
public:
    AgentPlanCommandCompiler();

    pathfinder::foundation::Result<WorldAgentCompiledCommand> compile(
        const std::string& actor_key,
        const pathfinder::agent_reasoning::AgentPlanStep& step,
        const WorldAgentDecisionContext& context,
        const std::string& tick_request_id);

private:
    WorldAgentStepCommandKind mapActionToStepKind(const std::string& action_key) const;
    pathfinder::world_command::WorldCommandKind mapStepToCommandKind(WorldAgentStepCommandKind step_kind) const;
    bool isAdjacent(const pathfinder::world_runtime::WorldCellCoord& from, const pathfinder::world_runtime::WorldCellCoord& to) const;
    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandDto> buildMoveCommand(
        const std::string& actor_key,
        const pathfinder::agent_reasoning::AgentPlanStep& step,
        const WorldAgentDecisionContext& context,
        const std::string& command_id) const;
    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandDto> buildGatherCommand(
        const std::string& actor_key,
        const pathfinder::agent_reasoning::AgentPlanStep& step,
        const std::string& command_id) const;
    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandDto> buildGenericCommand(
        const std::string& actor_key,
        const pathfinder::agent_reasoning::AgentPlanStep& step,
        WorldAgentStepCommandKind step_kind,
        const std::string& command_id) const;
};

} // namespace pathfinder::world_agent_execution
