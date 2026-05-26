#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_modules/core/world_use_command_router.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"

#include <memory>

namespace pathfinder::world_object_interaction {

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createEatObjectCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime = nullptr);

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createUseObjectCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry);

} // namespace pathfinder::world_object_interaction
