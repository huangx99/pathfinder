#pragma once

#include "pathfinder/client_protocol/client_protocol_types.h"
#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_modules/core/world_use_command_router.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <memory>
#include <string>

namespace pathfinder::world_npc_work {

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createNpcWorkUseCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_harvest::ResourceHarvestService& harvest_service,
    pathfinder::world_reaction::WorldReactionService& reaction_service,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository);

void runNpcWorkTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_harvest::ResourceHarvestService& harvest_service,
    pathfinder::world_reaction::WorldReactionService& reaction_service,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    pathfinder::client_protocol::ClientCommandResponse& response);

bool isNpcWorkActive(const std::string& actor_key);
void clearNpcWorkTasks();

} // namespace pathfinder::world_npc_work
