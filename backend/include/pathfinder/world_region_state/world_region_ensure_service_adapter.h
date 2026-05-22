#pragma once

#include "pathfinder/world_region_state/iworld_region_generation_ensure_port.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"

namespace pathfinder::world_region_state {

// P59: Adapter that makes P58 WorldRegionEnsureService usable as IWorldRegionGenerationEnsurePort.
// This keeps P58 policy (max_regions, coverage, warnings, ensure_kind) intact
// while allowing P59 lifecycle service to delegate generation fallback.
class WorldRegionEnsureServiceAdapter : public IWorldRegionGenerationEnsurePort {
public:
    explicit WorldRegionEnsureServiceAdapter(
        world_generation::WorldRegionEnsureService& ensure_service);

    pathfinder::foundation::Result<world_generation::WorldRegionEnsureResult> ensureRegions(
        const world_generation::WorldRegionEnsureRequest& request) override;

private:
    world_generation::WorldRegionEnsureService& ensure_service_;
};

} // namespace pathfinder::world_region_state
