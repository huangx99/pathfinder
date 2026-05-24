#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include <memory>

namespace pathfinder::world_generation {
class MoveTargetRegionGuard;
}

namespace pathfinder::world_runtime {

// P44 runtime-aware handlers (must be linked into pathfinder_world_command_runtime target)
std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createGenerateWorldCommandHandler(IWorldRuntime& runtime);
std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createMoveCommandHandler(IWorldRuntime& runtime, pathfinder::world_generation::MoveTargetRegionGuard* guard);
std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createInspectCommandHandler(IWorldRuntime& runtime);
std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createWaitCommandHandler(IWorldRuntime& runtime);
std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createAttackCommandHandler(IWorldRuntime& runtime);

} // namespace pathfinder::world_runtime
