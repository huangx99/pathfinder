#pragma once

#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_generation {

class WorldRegionEnsureService {
public:
    WorldRegionEnsureService(
        WorldGenerationService& generation_service,
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IWorldEntityLocationPort& location_port);

    pathfinder::foundation::Result<WorldRegionEnsureResult> ensureRegions(
        const WorldRegionEnsureRequest& request);

private:
    WorldGenerationService& generation_service_;
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;

    std::vector<WorldRegionCoord> computeDesiredRegions(
        const WorldRegionEnsureRequest& request);

    WorldRegionEnsureItemResult ensureSingleRegion(
        const WorldRegionEnsureRequest& request,
        const WorldRegionCoord& region);
};

} // namespace pathfinder::world_generation
