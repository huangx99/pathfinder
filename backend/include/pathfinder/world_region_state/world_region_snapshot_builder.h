#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_region_state/iworld_region_state_query_port.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: collect active runtime region -> snapshot
// Builder 只 collect/validate，不 detach runtime
class WorldRegionSnapshotBuilder {
public:
    explicit WorldRegionSnapshotBuilder(IWorldRegionStateQueryPort& query_port);

    foundation::Result<WorldRegionSnapshot> build(
        const world_generation::WorldRegionKey& region_key,
        const WorldRegionSnapshotBuildContext& context);

private:
    IWorldRegionStateQueryPort& query_port_;

    foundation::Result<void> validateSliceForRegion(
        const WorldRegionRuntimeSlice& slice,
        const world_generation::WorldRegionKey& region_key);
};

} // namespace pathfinder::world_region_state
