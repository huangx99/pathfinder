#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/world_modules/core/world_use_command_router.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <functional>
#include <memory>
#include <string>

namespace pathfinder::world_follow {

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createFollowUseCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime);

bool runFollowTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const std::string& leader_actor_key,
    pathfinder::client_protocol::ClientCommandResponse& response,
    const std::function<bool(const std::string&)>& is_actor_busy = {});

bool isFollowingActor(const std::string& actor_key);
void clearFollowingActors();

} // namespace pathfinder::world_follow
