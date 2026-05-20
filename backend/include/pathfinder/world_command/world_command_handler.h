#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_command {

class IWorldCommandHandler {
public:
    virtual ~IWorldCommandHandler() = default;
    virtual WorldCommandKind kind() const = 0;
    virtual pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const = 0;
};

} // namespace pathfinder::world_command
