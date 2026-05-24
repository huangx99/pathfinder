#include "pathfinder/world_modules/core/world_use_command_router.h"
#include <utility>

namespace pathfinder::world_modules {

pathfinder::world_command::WorldCommandKind WorldUseCommandRouter::kind() const {
    return pathfinder::world_command::WorldCommandKind::Use;
}

void WorldUseCommandRouter::addHandler(std::shared_ptr<IWorldUseCommandHandler> handler) {
    if (handler) handlers_.push_back(std::move(handler));
}

pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> WorldUseCommandRouter::execute(
    pathfinder::world_command::WorldCommandContext& context,
    const pathfinder::world_command::WorldCommandDto& command) const {
    for (const auto& handler : handlers_) {
        if (handler && handler->supports(command)) return handler->execute(context, command);
    }

    pathfinder::world_command::WorldCommandExecutionResult result;
    result.result_kind = pathfinder::world_command::WorldCommandResultKind::Blocked;
    result.failure_reason_keys.push_back("unsupported_use_command");
    return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
}

} // namespace pathfinder::world_modules
