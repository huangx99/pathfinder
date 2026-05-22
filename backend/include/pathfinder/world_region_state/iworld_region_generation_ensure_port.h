#pragma once

#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: Abstract port for generation fallback, implemented by P58 WorldRegionEnsureService.
// This prevents P59 lifecycle service from directly calling WorldGenerationService
// and ensures P58 policy (max_regions, coverage, warnings, etc.) is preserved.
class IWorldRegionGenerationEnsurePort {
public:
    virtual ~IWorldRegionGenerationEnsurePort() = default;

    virtual pathfinder::foundation::Result<world_generation::WorldRegionEnsureResult> ensureRegions(
        const world_generation::WorldRegionEnsureRequest& request) = 0;
};

} // namespace pathfinder::world_region_state
