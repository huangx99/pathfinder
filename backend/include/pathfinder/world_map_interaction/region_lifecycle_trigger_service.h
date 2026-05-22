#pragma once

#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "pathfinder/world_region_state/world_region_lifecycle_service.h"
#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/foundation/result.h"
#include <string>
#include <vector>

namespace pathfinder::world_map_interaction {

class RegionActivityWindowService;

// ---------------------------------------------------------------------------
// RegionLifecycleTriggerService
// ---------------------------------------------------------------------------
// P60: Executes automatic save triggers by delegating to P59 lifecycle service.
// Does not directly write snapshots; only orchestrates seal/restore/detach.
// ---------------------------------------------------------------------------

class RegionLifecycleTriggerService {
public:
    struct TriggerResult {
        std::vector<ClientRegionLifecycleEventDto> events;
        std::vector<std::string> warning_keys;
        std::vector<std::string> failure_reason_keys;
    };

    explicit RegionLifecycleTriggerService(
        world_region_state::WorldRegionLifecycleService& lifecycle_service);

    void setActivityWindowService(RegionActivityWindowService* service);

    // Process a computed activity window: seal candidates, detach candidates,
    // and ensure prewarm regions are available.
    TriggerResult processWindow(
        const RegionActivityWindow& window,
        const world_region_state::WorldRegionSnapshotBuildContext& context);

    // Explicitly ensure a region is available (for move targets, selection, etc.)
    // Returns true if the region is available (active or restored/generated).
    // Returns false if restore failed (does NOT silently generate over failed restore).
    pathfinder::foundation::Result<bool> ensureRegionAvailable(
        const world_generation::WorldRegionKey& region_key,
        const world_region_state::WorldRegionSnapshotBuildContext& context);

private:
    world_region_state::WorldRegionLifecycleService& lifecycle_service_;
    RegionActivityWindowService* activity_window_service_ = nullptr;

    ClientRegionLifecycleEventDto makeEvent(
        const std::string& event_kind,
        const world_generation::WorldRegionKey& region_key,
        const std::vector<std::string>& failure_reason_keys = {},
        const std::vector<std::string>& warning_keys = {});

    std::string regionLabel(const world_generation::WorldRegionKey& key) const;
};

} // namespace pathfinder::world_map_interaction
