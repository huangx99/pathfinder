#include "pathfinder/world_modules/core/world_module_context.h"
#include <utility>

namespace pathfinder::world_modules {

void WorldPostCommandHookRegistry::addHook(std::string hook_key, WorldPostCommandHook hook) {
    if (!hook) return;
    hooks_.push_back({std::move(hook_key), std::move(hook)});
}

void WorldPostCommandHookRegistry::runHooks(
    const pathfinder::world_command::WorldCommandDto& command,
    pathfinder::client_protocol::ClientCommandResponse& response) const {
    for (const auto& [key, hook] : hooks_) {
        (void)key;
        hook(command, response);
    }
}

void WorldPostCommandHookRegistry::clear() {
    hooks_.clear();
}

} // namespace pathfinder::world_modules
