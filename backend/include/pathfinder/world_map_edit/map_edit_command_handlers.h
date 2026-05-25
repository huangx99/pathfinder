#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include <memory>

namespace pathfinder::world_map_edit {

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createPaintTerrainCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& world_runtime);

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createSpawnEntityCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& world_runtime,
    pathfinder::world_inventory::IWorldEntityLocationPort& location_port,
    const pathfinder::content::ContentRegistry& content_registry);

} // namespace pathfinder::world_map_edit
