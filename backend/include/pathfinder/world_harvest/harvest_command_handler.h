#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_harvest {
class ResourceHarvestService;
}

namespace pathfinder::world_command {

// P47: Gather/Chop/Mine/Dig command handler factories
std::shared_ptr<IWorldCommandHandler> createGatherCommandHandler(
    world_harvest::ResourceHarvestService& service);

std::shared_ptr<IWorldCommandHandler> createChopCommandHandler(
    world_harvest::ResourceHarvestService& service);

std::shared_ptr<IWorldCommandHandler> createMineCommandHandler(
    world_harvest::ResourceHarvestService& service);

std::shared_ptr<IWorldCommandHandler> createDigCommandHandler(
    world_harvest::ResourceHarvestService& service);

} // namespace pathfinder::world_command
