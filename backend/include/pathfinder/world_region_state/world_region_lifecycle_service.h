#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_region_state/iworld_region_state_store.h"
#include "pathfinder/world_region_state/iworld_region_state_query_port.h"
#include "pathfinder/world_region_state/iworld_region_state_apply_port.h"
#include "pathfinder/world_region_state/world_region_snapshot_builder.h"
#include "pathfinder/world_region_state/world_region_snapshot_restorer.h"
#include "pathfinder/world_region_state/world_region_unload_guard.h"
#include "pathfinder/world_region_state/iworld_region_generation_ensure_port.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/foundation/result.h"
#include <map>
#include <mutex>

namespace pathfinder::world_region_state {

// P59: seal/cache/restore/availability 调度
// ensureAvailable 顺序固定：
//   if runtime active -> ActiveRuntime
//   else if store has snapshot -> restore -> RestoredSnapshot
//   else -> delegate P58 generation -> GeneratedBaseline
class WorldRegionLifecycleService {
public:
    // P59: Full constructor with generation ensure port (preferred).
    WorldRegionLifecycleService(
        IWorldRegionStateStore& store,
        IWorldRegionStateQueryPort& query_port,
        IWorldRegionStateApplyPort& apply_port,
        world_inventory::IWorldEntityLocationPort& location_port,
        world_runtime::IWorldRuntime& world_runtime,
        IWorldRegionGenerationEnsurePort* generation_ensure_port);

    // P59: Backward-compatible constructor for tests that don't need generation fallback.
    WorldRegionLifecycleService(
        IWorldRegionStateStore& store,
        IWorldRegionStateQueryPort& query_port,
        IWorldRegionStateApplyPort& apply_port,
        world_inventory::IWorldEntityLocationPort& location_port,
        world_runtime::IWorldRuntime& world_runtime);

    // Seal an active region into snapshot store
    foundation::Result<WorldRegionSealResult> sealRegion(
        const world_generation::WorldRegionKey& region_key,
        SealMode mode = SealMode::DetachSafeOnly);

    // Restore a region from snapshot store to runtime
    foundation::Result<WorldRegionRestoreResult> restoreRegion(
        const world_generation::WorldRegionKey& region_key);

    // Ensure region is available: active -> restored -> generated
    foundation::Result<WorldRegionAvailabilityResult> ensureAvailable(
        const world_generation::WorldRegionKey& region_key,
        const WorldRegionSnapshotBuildContext& context);

    // Get current lifecycle state
    WorldRegionLifecycleState getLifecycleState(
        const world_generation::WorldRegionKey& region_key) const;

    // Explicitly set lifecycle state (for testing/debug)
    void setLifecycleState(
        const world_generation::WorldRegionKey& region_key,
        WorldRegionLifecycleState state);

    // Check if region is currently active in runtime
    bool isRegionActive(const world_generation::WorldRegionKey& region_key) const;

    // Get all cached region keys
    std::vector<world_generation::WorldRegionKey> cachedRegionKeys() const;

private:
    IWorldRegionStateStore& store_;
    IWorldRegionStateQueryPort& query_port_;
    IWorldRegionStateApplyPort& apply_port_;
    world_inventory::IWorldEntityLocationPort& location_port_;
    world_runtime::IWorldRuntime& world_runtime_;
    IWorldRegionGenerationEnsurePort* generation_ensure_port_ = nullptr;

    mutable std::mutex lifecycle_mutex_;
    std::map<std::string, WorldRegionLifecycleState> lifecycle_states_;

    WorldRegionUnloadGuard unload_guard_;

    std::string makeLifecycleKey(const world_generation::WorldRegionKey& key) const;

    foundation::Result<WorldRegionAvailabilityResult> ensureViaGeneration(
        const world_generation::WorldRegionKey& region_key,
        const WorldRegionSnapshotBuildContext& context);
};

} // namespace pathfinder::world_region_state
