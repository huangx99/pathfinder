#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"

namespace pathfinder::world_beast_ecology {

void runWildlifeTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    pathfinder::client_protocol::ClientCommandResponse& response);

} // namespace pathfinder::world_beast_ecology
