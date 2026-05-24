#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include <memory>
#include <vector>

namespace pathfinder::world_modules {

class IWorldUseCommandHandler {
public:
    virtual ~IWorldUseCommandHandler() = default;
    virtual bool supports(const pathfinder::world_command::WorldCommandDto& command) const = 0;
    virtual pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const = 0;
};

class WorldUseCommandRouter final : public pathfinder::world_command::IWorldCommandHandler {
public:
    pathfinder::world_command::WorldCommandKind kind() const override;
    void addHandler(std::shared_ptr<IWorldUseCommandHandler> handler);

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override;

private:
    std::vector<std::shared_ptr<IWorldUseCommandHandler>> handlers_;
};

} // namespace pathfinder::world_modules
