#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_command/world_command_types.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// WorldGenerationProjectionBridge - converts generation result to projection patch
// ---------------------------------------------------------------------------

class WorldGenerationProjectionBridge {
public:
    static world_command::WorldProjectionPatchDto buildPatch(
        const WorldGenerationApplyResult& apply_result,
        uint64_t base_projection_version);

    static std::vector<world_command::WorldEventDto> buildEvents(
        const WorldGenerationResult& result);
};

} // namespace pathfinder::world_generation
