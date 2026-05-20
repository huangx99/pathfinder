#pragma once

#include "pathfinder/world_harvest/world_harvest_types.h"
#include "pathfinder/world_command/world_command_types.h"

namespace pathfinder::world_harvest {

// ---------------------------------------------------------------------------
// HarvestProjectionBridge - converts harvest result to projection patch
// ---------------------------------------------------------------------------

class HarvestProjectionBridge {
public:
    static world_command::WorldProjectionPatchDto buildPatch(
        const ResourceHarvestResult& result,
        uint64_t base_projection_version);
};

} // namespace pathfinder::world_harvest
