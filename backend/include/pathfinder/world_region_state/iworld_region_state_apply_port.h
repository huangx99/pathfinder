#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: 把已验证 snapshot 恢复到 runtime，做冲突检查
// Restore plan 必须先检查：region active conflict, cell id conflict, entity id conflict,
// resource node id conflict, OnMap ownership violation, state version/version compatibility
class IWorldRegionStateApplyPort {
public:
    virtual ~IWorldRegionStateApplyPort() = default;

    virtual foundation::Result<WorldRegionApplyPlan> planRestore(
        const WorldRegionSnapshot& snapshot) = 0;

    virtual foundation::Result<WorldRegionRestoreResult> applyRestore(
        const WorldRegionApplyPlan& plan) = 0;
};

} // namespace pathfinder::world_region_state
