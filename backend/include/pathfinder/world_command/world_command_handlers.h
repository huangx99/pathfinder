#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_command {

// P43 stub handlers (no runtime dependency)
std::shared_ptr<IWorldCommandHandler> createNoopCommandHandler();
std::shared_ptr<IWorldCommandHandler> createWaitCommandHandler();
std::shared_ptr<IWorldCommandHandler> createInspectCommandHandler();
std::shared_ptr<IWorldCommandHandler> createSystemTickCommandHandler();
std::shared_ptr<IWorldCommandHandler> createGenerateWorldCommandHandler();

} // namespace pathfinder::world_command
