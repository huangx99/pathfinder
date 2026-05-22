#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_region_state/iworld_region_state_apply_port.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: snapshot -> runtime apply plan/result
// Restorer 不调用生成器
class WorldRegionSnapshotRestorer {
public:
    explicit WorldRegionSnapshotRestorer(IWorldRegionStateApplyPort& apply_port);

    foundation::Result<WorldRegionRestoreResult> restore(
        const WorldRegionSnapshot& snapshot,
        const WorldRegionRestoreContext& context);

private:
    IWorldRegionStateApplyPort& apply_port_;
};

} // namespace pathfinder::world_region_state
