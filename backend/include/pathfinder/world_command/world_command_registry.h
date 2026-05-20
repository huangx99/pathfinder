#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/foundation/result.h"
#include <map>
#include <memory>
#include <string>

namespace pathfinder::world_command {

class WorldCommandHandlerRegistry {
public:
    WorldCommandHandlerRegistry() = default;

    pathfinder::foundation::Result<void> registerHandler(
        std::shared_ptr<IWorldCommandHandler> handler);

    pathfinder::foundation::Result<void> bindActionKey(
        const std::string& action_key,
        WorldCommandKind handler_kind);

    const IWorldCommandHandler* findByKind(WorldCommandKind kind) const;
    const IWorldCommandHandler* findByActionKey(const std::string& action_key) const;

    size_t handlerCount() const;
    void clear();

private:
    std::map<WorldCommandKind, std::shared_ptr<IWorldCommandHandler>> kind_registry_;
    std::map<std::string, std::shared_ptr<IWorldCommandHandler>> action_registry_;
};

} // namespace pathfinder::world_command
