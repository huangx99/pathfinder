#pragma once

#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_region_state {

// P59: cache/store 抽象，第一版 in-memory
// Store 抽象存在的原因：P59 先保证 cache roundtrip；后续写 P30 bridge 或磁盘 store 时不改 lifecycle service
class IWorldRegionStateStore {
public:
    virtual ~IWorldRegionStateStore() = default;

    virtual foundation::Result<void> put(const WorldRegionSnapshot& snapshot) = 0;
    virtual foundation::Result<const WorldRegionSnapshot*> find(
        const world_generation::WorldRegionKey& key) const = 0;
    virtual bool has(const world_generation::WorldRegionKey& key) const = 0;
    virtual foundation::Result<void> remove(const world_generation::WorldRegionKey& key) = 0;
    virtual std::vector<world_generation::WorldRegionKey> keys() const = 0;
    virtual std::size_t count() const = 0;
    virtual void clear() = 0;
};

} // namespace pathfinder::world_region_state
