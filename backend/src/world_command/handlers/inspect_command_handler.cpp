#include "pathfinder/world_command/world_command_handlers.h"
#include <memory>

namespace pathfinder::world_command {

class InspectCommandHandlerStub : public IWorldCommandHandler {
public:
    WorldCommandKind kind() const override {
        return WorldCommandKind::Inspect;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_inspect";
        event.event_kind = "Inspect";
        event.tick = command.context.issued_tick;
        event.title_text = "观察";
        event.body_text = "你仔细观察了目标。";
        event.actor_key = command.actor_key;
        event.target_entity_id = command.target.target_entity_id;
        result.events.push_back(std::move(event));

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
};

std::shared_ptr<IWorldCommandHandler> createInspectCommandHandler() {
    return std::make_shared<InspectCommandHandlerStub>();
}

} // namespace pathfinder::world_command
