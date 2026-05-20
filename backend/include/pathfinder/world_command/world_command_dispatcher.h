#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_command {

class WorldCommandDispatcher {
public:
    explicit WorldCommandDispatcher(WorldCommandHandlerRegistry& registry);

    pathfinder::foundation::Result<WorldCommandExecutionResult> dispatch(
        WorldCommandContext& context,
        const WorldCommandDto& command) const;

private:
    WorldCommandHandlerRegistry& registry_;
};

} // namespace pathfinder::world_command
