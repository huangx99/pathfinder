#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/terrain_noise_sampler.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainNoiseFieldBuilder - builds a multi-channel noise field for a region
// ---------------------------------------------------------------------------

class TerrainNoiseFieldBuilder {
public:
    static TerrainNoiseField build(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile);
};

} // namespace pathfinder::world_generation
