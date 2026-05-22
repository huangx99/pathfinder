#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_region_state/iworld_region_state_store.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_region_state {

// P59: Collection of region snapshots for save/restore bridge
struct WorldRegionSnapshotCollection {
    std::vector<WorldRegionSnapshot> region_snapshots;
    uint64_t world_tick = 0;
    uint64_t state_version = 0;
    std::string world_id;
    std::string generator_version;
    std::string content_version;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// P59: Section draft for later integration with P30 SaveGamePackage
struct WorldRegionSaveSectionDraft {
    std::string section_key = "world_region_snapshot.v1";
    WorldRegionSnapshotCollection collection;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// P59: Bridge between region snapshot store and P30 SaveGamePackage
// P59 推荐做到 DTO 和桥接口；后续 P30/P55 存档阶段把 section 放入 SaveGamePackage
class WorldRegionSaveBridge {
public:
    // Build a section draft from all snapshots in the store
    pathfinder::foundation::Result<WorldRegionSaveSectionDraft> buildSectionDraft(
        const IWorldRegionStateStore& store,
        const std::string& world_id,
        uint64_t world_tick,
        uint64_t state_version) const;

    // Restore snapshots from a section draft into the store
    pathfinder::foundation::Result<std::vector<world_generation::WorldRegionKey>> restoreStoreFromSection(
        IWorldRegionStateStore& store,
        const WorldRegionSaveSectionDraft& draft) const;

    // Validate that a collection can be safely restored
    pathfinder::foundation::Result<void> validateCollection(
        const WorldRegionSnapshotCollection& collection) const;
};

} // namespace pathfinder::world_region_state
