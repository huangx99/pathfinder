#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_command {

class SystemTickCommandHandler : public IWorldCommandHandler {
public:
    WorldCommandKind kind() const override {
        return WorldCommandKind::SystemTick;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_tick";
        event.event_kind = "SystemTick";
        event.tick = command.context.issued_tick;
        event.title_text = "系统推进";
        event.body_text = "世界时间向前推进了一刻。";
        result.events.push_back(std::move(event));

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
};

std::shared_ptr<IWorldCommandHandler> createSystemTickCommandHandler() {
    return std::make_shared<SystemTickCommandHandler>();
}

} // namespace pathfinder::world_command
