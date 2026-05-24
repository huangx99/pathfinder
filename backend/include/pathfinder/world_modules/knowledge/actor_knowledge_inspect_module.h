#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include <memory>

namespace pathfinder::world_modules {

std::shared_ptr<pathfinder::world_command::IWorldCommandHandler> createActorKnowledgeInspectCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& runtime,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    const pathfinder::content::ContentRegistry& content_registry);

} // namespace pathfinder::world_modules
