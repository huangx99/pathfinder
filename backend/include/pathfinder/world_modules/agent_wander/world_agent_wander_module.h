#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <functional>
#include <string>

namespace pathfinder::world_agent_wander {

bool runAgentWanderTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    const std::function<bool(const std::string&)>& is_actor_busy,
    pathfinder::client_protocol::ClientCommandResponse& response);

void clearAgentWanderRuntimeState();

} // namespace pathfinder::world_agent_wander
