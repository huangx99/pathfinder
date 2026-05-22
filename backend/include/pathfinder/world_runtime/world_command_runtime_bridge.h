#pragma once

#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_runtime {

class WorldCommandRuntimeBridge {
public:
    explicit WorldCommandRuntimeBridge(IWorldRuntime& runtime);

    foundation::Result<world_command::WorldCommandExecutionResult> handleGenerateWorld(
        const world_command::WorldCommandContext& context,
        const world_command::WorldCommandDto& command);

    foundation::Result<world_command::WorldCommandExecutionResult> handleMove(
        const world_command::WorldCommandContext& context,
        const world_command::WorldCommandDto& command);

    foundation::Result<world_command::WorldCommandExecutionResult> handleInspect(
        const world_command::WorldCommandContext& context,
        const world_command::WorldCommandDto& command);

    foundation::Result<world_command::WorldCommandExecutionResult> handleWait(
        const world_command::WorldCommandContext& context,
        const world_command::WorldCommandDto& command);

    // P58: Accessors for command handlers that need runtime config.
    std::string worldId() const { return runtime_.worldId(); }
    uint64_t worldSeed() const { return runtime_.worldSeed(); }
    int regionSize() const { return runtime_.regionSize(); }

    // Helper to build projection patch for a command result
    world_command::WorldProjectionPatchDto buildProjectionPatch(
        const std::vector<std::string>& changed_cell_ids,
        const std::vector<std::string>& changed_entity_ids,
        const std::string& viewer_actor_key,
        bool requires_full_refresh = false);

private:
    IWorldRuntime& runtime_;
    WorldProjectionAdapter projection_adapter_;
};

} // namespace pathfinder::world_runtime
