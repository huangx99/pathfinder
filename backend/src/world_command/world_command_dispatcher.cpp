#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_command {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

WorldCommandDispatcher::WorldCommandDispatcher(WorldCommandHandlerRegistry& registry)
    : registry_(registry) {}

Result<WorldCommandExecutionResult> WorldCommandDispatcher::dispatch(
    WorldCommandContext& context,
    const WorldCommandDto& command) const {

    const IWorldCommandHandler* handler = registry_.findByKind(command.command_kind);
    if (!handler) {
        WorldCommandExecutionResult failure;
        failure.result_kind = WorldCommandResultKind::Failed;
        failure.failure_reason_keys.push_back("unknown_command");
        return Result<WorldCommandExecutionResult>::ok(std::move(failure));
    }

    return handler->execute(context, command);
}

} // namespace pathfinder::world_command
