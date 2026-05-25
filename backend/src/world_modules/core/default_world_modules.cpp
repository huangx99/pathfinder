#include "pathfinder/world_modules/core/default_world_modules.h"
#include "pathfinder/world_beast_ecology/beast_runtime_module.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_modules/follow/world_follow_module.h"
#include "pathfinder/world_generation/world_generation_command_handler.h"
#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include "pathfinder/world_modules/knowledge/actor_knowledge_inspect_module.h"
#include "pathfinder/world_modules/core/world_use_command_router.h"
#include "pathfinder/world_modules/npc_work/world_npc_work_module.h"
#include "pathfinder/world_reaction/craft_command_handler.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include "pathfinder/world_teaching/teach_command_handler.h"
#include "pathfinder/world_map_edit/map_edit_command_handlers.h"
#include <memory>
#include <optional>

namespace pathfinder::world_modules {
namespace {

class RuntimeActorQueryPort final : public pathfinder::world_teaching::IWorldActorQueryPort {
public:
    explicit RuntimeActorQueryPort(pathfinder::world_runtime::IWorldRuntime& runtime) : runtime_(runtime) {}

    std::optional<pathfinder::world_runtime::WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto res = runtime_.findActor(actor_key);
        if (res.is_error()) return std::nullopt;
        return *res.value();
    }

private:
    pathfinder::world_runtime::IWorldRuntime& runtime_;
};

void registerCoreCommandHandlers(WorldModuleContext& context) {
    context.registry.registerHandler(pathfinder::world_command::createGenerateWorldCommandHandler(
        context.worldgen_service, context.world_runtime, context.world_runtime));
    context.registry.registerHandler(pathfinder::world_runtime::createMoveCommandHandler(
        context.world_runtime, context.move_guard));
    context.registry.registerHandler(pathfinder::world_runtime::createWaitCommandHandler(context.world_runtime));
    context.registry.registerHandler(pathfinder::world_runtime::createAttackCommandHandler(context.world_runtime));

    context.registry.registerHandler(pathfinder::world_map_edit::createPaintTerrainCommandHandler(
        context.world_runtime));
    context.registry.registerHandler(pathfinder::world_map_edit::createSpawnEntityCommandHandler(
        context.world_runtime, context.world_runtime, context.content_registry));

    context.registry.registerHandler(pathfinder::world_command::createGatherCommandHandler(context.harvest_service));
    context.registry.registerHandler(pathfinder::world_command::createChopCommandHandler(context.harvest_service));
    context.registry.registerHandler(pathfinder::world_command::createMineCommandHandler(context.harvest_service));
    context.registry.registerHandler(pathfinder::world_command::createDigCommandHandler(context.harvest_service));

    context.registry.registerHandler(pathfinder::world_inventory::createPickupCommandHandler(
        context.inventory_runtime, context.world_runtime));
    context.registry.registerHandler(pathfinder::world_inventory::createDropCommandHandler(
        context.inventory_runtime, context.world_runtime));

    context.registry.registerHandler(pathfinder::world_reaction::createCraftCommandHandler(
        context.reaction_service, context.world_runtime, context.inventory_runtime));

    context.registry.registerHandler(pathfinder::world_teaching::createTeachCommandHandler(
        context.knowledge_repository,
        std::make_unique<RuntimeActorQueryPort>(context.world_runtime),
        &context.content_registry));

    context.registry.registerHandler(createActorKnowledgeInspectCommandHandler(
        context.world_runtime, context.knowledge_repository, context.content_registry));
}

void registerUseCommandHandlers(WorldModuleContext& context) {
    auto use_router = std::make_shared<WorldUseCommandRouter>();
    use_router->addHandler(pathfinder::world_follow::createFollowUseCommandHandler(context.world_runtime));
    use_router->addHandler(pathfinder::world_npc_work::createNpcWorkUseCommandHandler(
        context.world_runtime,
        context.inventory_runtime,
        context.harvest_service,
        context.reaction_service,
        context.content_registry,
        context.knowledge_repository));
    context.registry.registerHandler(use_router);
}

void registerPostCommandHooks(WorldModuleContext& context) {
    auto* world_runtime = &context.world_runtime;
    auto* inventory_runtime = &context.inventory_runtime;
    auto* harvest_service = &context.harvest_service;
    auto* reaction_service = &context.reaction_service;
    auto* content_registry = &context.content_registry;
    auto* knowledge_repository = &context.knowledge_repository;
    auto* pipeline = &context.pipeline;

    context.post_command_hooks.addHook("world_npc_work", [world_runtime,
                                                           inventory_runtime,
                                                           harvest_service,
                                                           reaction_service,
                                                           content_registry,
                                                           knowledge_repository](
        const pathfinder::world_command::WorldCommandDto& command,
        pathfinder::client_protocol::ClientCommandResponse& response) {
        if (command.command_kind != pathfinder::world_command::WorldCommandKind::Wait) return;
        if (response.result.result_kind != pathfinder::world_command::WorldCommandResultKind::Succeeded) return;
        pathfinder::world_npc_work::runNpcWorkTicks(
            *world_runtime,
            *inventory_runtime,
            *harvest_service,
            *reaction_service,
            *content_registry,
            *knowledge_repository,
            response);
    });

    context.post_command_hooks.addHook("world_follow", [world_runtime](
        const pathfinder::world_command::WorldCommandDto& command,
        pathfinder::client_protocol::ClientCommandResponse& response) {
        if (command.actor_key.empty()) return;
        (void)pathfinder::world_follow::runFollowTicks(
            *world_runtime,
            command.actor_key,
            response,
            [](const std::string& actor_key) {
                return pathfinder::world_npc_work::isNpcWorkActive(actor_key);
            });
    });

    context.post_command_hooks.addHook("world_beast_ecology", [world_runtime,
                                                               content_registry,
                                                               knowledge_repository,
                                                               pipeline](
        const pathfinder::world_command::WorldCommandDto& command,
        pathfinder::client_protocol::ClientCommandResponse& response) {
        if (command.command_kind != pathfinder::world_command::WorldCommandKind::Wait) return;
        pathfinder::world_beast_ecology::runWildlifeTicks(
            *world_runtime,
            *content_registry,
            *knowledge_repository,
            *pipeline,
            response);
    });
}

} // namespace

void registerDefaultWorldModules(WorldModuleContext& context) {
    registerCoreCommandHandlers(context);
    registerUseCommandHandlers(context);
    registerPostCommandHooks(context);

    // Base system handlers are last-resort infrastructure handlers only. Playable
    // behavior must still be supplied by a runtime-aware module registered above.
    context.registry.registerHandler(pathfinder::world_command::createNoopCommandHandler());
    context.registry.registerHandler(pathfinder::world_command::createSystemTickCommandHandler());
}

void clearDefaultWorldModuleRuntimeState() {
    pathfinder::world_follow::clearFollowingActors();
    pathfinder::world_npc_work::clearNpcWorkTasks();
}

} // namespace pathfinder::world_modules
