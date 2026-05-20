#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include <memory>

namespace pathfinder::world_inventory {

std::shared_ptr<world_command::IWorldCommandHandler> createPickupCommandHandler(
    IInventoryRuntime& inventory_runtime,
    world_runtime::IWorldRuntime& world_runtime);

std::shared_ptr<world_command::IWorldCommandHandler> createDropCommandHandler(
    IInventoryRuntime& inventory_runtime,
    world_runtime::IWorldRuntime& world_runtime);

} // namespace pathfinder::world_inventory
