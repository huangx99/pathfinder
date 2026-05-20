#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_generation {
class WorldGenerationService;
}

namespace pathfinder::world_runtime {
class IWorldRuntime;
}

namespace pathfinder::world_inventory {
class IWorldEntityLocationPort;
}

namespace pathfinder::world_command {

// P46: runtime-aware world generation handler
std::shared_ptr<IWorldCommandHandler> createGenerateWorldCommandHandler(
    std::shared_ptr<world_generation::WorldGenerationService> service,
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port);

} // namespace pathfinder::world_command
