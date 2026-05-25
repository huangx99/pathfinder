#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include <vector>

namespace pathfinder::world_map_edit {

class MapEditCommandOptionProvider {
public:
    MapEditCommandOptionProvider(
        const pathfinder::world_command::WorldCommandHandlerRegistry& registry,
        const pathfinder::content::ContentRegistry& content_registry);

    std::vector<pathfinder::world_command::WorldCommandOptionDto> buildOptions(
        const pathfinder::world_runtime::WorldActorRuntime& actor,
        const pathfinder::world_runtime::IWorldRuntime& runtime) const;

private:
    const pathfinder::world_command::WorldCommandHandlerRegistry& registry_;
    const pathfinder::content::ContentRegistry& content_registry_;
};

} // namespace pathfinder::world_map_edit
