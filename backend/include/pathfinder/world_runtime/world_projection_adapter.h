#pragma once

#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_runtime {

class WorldProjectionAdapter {
public:
    foundation::Result<std::vector<world_command::WorldCellPatchDto>> buildCellPatches(
        const std::vector<std::string>& changed_cell_ids,
        const std::string& viewer_actor_key,
        const IWorldRuntime& runtime) const;

    foundation::Result<std::vector<world_command::WorldEntityPatchDto>> buildEntityPatches(
        const std::vector<std::string>& changed_entity_ids,
        const std::string& viewer_actor_key,
        const IWorldRuntime& runtime) const;
};

} // namespace pathfinder::world_runtime
