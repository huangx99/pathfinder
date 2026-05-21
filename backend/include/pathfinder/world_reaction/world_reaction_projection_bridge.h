#pragma once

#include "pathfinder/world_reaction/world_reaction_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_command/world_command_types.h"

namespace pathfinder::world_reaction {

// ---------------------------------------------------------------------------
// WorldReactionProjectionBridge - converts reaction result to projection patch
// ---------------------------------------------------------------------------

class WorldReactionProjectionBridge {
public:
    static world_command::WorldProjectionPatchDto buildPatch(
        const WorldReactionResult& result,
        const std::string& viewer_actor_key,
        const world_runtime::IWorldRuntime& world_runtime,
        const world_inventory::IInventoryRuntime& inventory_runtime,
        uint64_t base_projection_version = 0);
};

} // namespace pathfinder::world_reaction
