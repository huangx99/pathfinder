#pragma once

#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pathfinder::world_modules {

using WorldPostCommandHook = std::function<void(
    const pathfinder::world_command::WorldCommandDto&,
    pathfinder::client_protocol::ClientCommandResponse&)>;

class WorldPostCommandHookRegistry {
public:
    void addHook(std::string hook_key, WorldPostCommandHook hook);
    void runHooks(
        const pathfinder::world_command::WorldCommandDto& command,
        pathfinder::client_protocol::ClientCommandResponse& response) const;
    void clear();

private:
    std::vector<std::pair<std::string, WorldPostCommandHook>> hooks_;
};

struct WorldModuleContext {
    pathfinder::world_command::WorldCommandHandlerRegistry& registry;
    pathfinder::world_command::WorldCommandPipeline& pipeline;
    std::shared_ptr<pathfinder::world_generation::WorldGenerationService> worldgen_service;
    pathfinder::world_generation::MoveTargetRegionGuard* move_guard = nullptr;
    pathfinder::world_runtime::WorldGridRuntime& world_runtime;
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime;
    pathfinder::world_harvest::ResourceHarvestService& harvest_service;
    pathfinder::world_reaction::WorldReactionService& reaction_service;
    const pathfinder::content::ContentRegistry& content_registry;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository;
    WorldPostCommandHookRegistry& post_command_hooks;
};

} // namespace pathfinder::world_modules
