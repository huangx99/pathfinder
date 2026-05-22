#include "pathfinder/world_region_state/world_region_ensure_service_adapter.h"

namespace pathfinder::world_region_state {

WorldRegionEnsureServiceAdapter::WorldRegionEnsureServiceAdapter(
    world_generation::WorldRegionEnsureService& ensure_service)
    : ensure_service_(ensure_service) {
}

pathfinder::foundation::Result<world_generation::WorldRegionEnsureResult>
WorldRegionEnsureServiceAdapter::ensureRegions(
    const world_generation::WorldRegionEnsureRequest& request) {
    return ensure_service_.ensureRegions(request);
}

} // namespace pathfinder::world_region_state
