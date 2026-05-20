#include "pathfinder/world_command/world_command_handler.h"
#include <memory>

namespace pathfinder::world_command {

class GenerateWorldCommandHandler : public IWorldCommandHandler {
public:
    WorldCommandKind kind() const override {
        return WorldCommandKind::GenerateWorld;
    }

    pathfinder::foundation::Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& /*context*/,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_gen";
        event.event_kind = "GenerateWorld";
        event.tick = command.context.issued_tick;
        event.title_text = "世界生成";
        event.body_text = "世界生成命令已执行（stub）。";
        result.events.push_back(std::move(event));

        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
};

std::shared_ptr<IWorldCommandHandler> createGenerateWorldCommandHandler() {
    return std::make_shared<GenerateWorldCommandHandler>();
}

} // namespace pathfinder::world_command
