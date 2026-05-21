#pragma once

#include <memory>
#include "pathfinder/world_command/world_command_handler.h"

namespace pathfinder::world_runtime { class IWorldRuntime; }
namespace pathfinder::world_inventory { class IInventoryRuntime; }

namespace pathfinder::world_reaction {

class WorldReactionService;

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createCraftCommandHandler(
    WorldReactionService& service,
    pathfinder::world_runtime::IWorldRuntime& world_runtime,
    pathfinder::world_inventory::IInventoryRuntime& inventory_runtime);

} // namespace pathfinder::world_reaction
