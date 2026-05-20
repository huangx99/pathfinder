#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_command {

class NoopCommandHandler : public IWorldCommandHandler {
public:
    WorldCommandKind kind() const override {
        return WorldCommandKind::Noop;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Noop;

        WorldEventDto event;
        event.event_id = command.command_id + "_noop";
        event.event_kind = "Noop";
        event.tick = command.context.issued_tick;
        event.title_text = "无操作";
        event.body_text = "命令已接收，但未执行任何操作。";
        event.actor_key = command.actor_key;
        result.events.push_back(std::move(event));

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
};

// Factory function for registration
std::shared_ptr<IWorldCommandHandler> createNoopCommandHandler() {
    return std::make_shared<NoopCommandHandler>();
}

} // namespace pathfinder::world_command
