#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_command {

class WaitCommandHandler : public IWorldCommandHandler {
public:
    WorldCommandKind kind() const override {
        return WorldCommandKind::Wait;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_wait";
        event.event_kind = "TimePassed";
        event.tick = command.context.issued_tick;
        event.title_text = "时间流逝";
        event.body_text = "你等待了一会儿。";
        event.actor_key = command.actor_key;
        result.events.push_back(std::move(event));

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
};

std::shared_ptr<IWorldCommandHandler> createWaitCommandHandler() {
    return std::make_shared<WaitCommandHandler>();
}

} // namespace pathfinder::world_command
