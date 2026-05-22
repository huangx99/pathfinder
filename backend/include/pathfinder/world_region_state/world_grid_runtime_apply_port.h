#pragma once

#include "pathfinder/world_region_state/iworld_region_state_apply_port.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"

namespace pathfinder::world_region_state {

// P59: IWorldRegionStateApplyPort implementation backed by WorldGridRuntime
// Uses IWorldRuntime and IWorldEntityLocationPort to apply restored snapshot data.
class WorldGridRuntimeApplyPort : public IWorldRegionStateApplyPort {
public:
    WorldGridRuntimeApplyPort(
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IWorldEntityLocationPort& location_port);

    foundation::Result<WorldRegionApplyPlan> planRestore(
        const WorldRegionSnapshot& snapshot) override;

    foundation::Result<WorldRegionRestoreResult> applyRestore(
        const WorldRegionApplyPlan& plan) override;

private:
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;

    foundation::Result<void> validateNoActiveConflict(
        const world_generation::WorldRegionKey& region_key);

    foundation::Result<void> validateEntityOwnership(
        const std::vector<WorldRegionEntityStateRecord>& entities);
};

} // namespace pathfinder::world_region_state
