#pragma once

#include "pathfinder/world_region_state/iworld_region_state_store.h"
#include <map>
#include <memory>
#include <mutex>

namespace pathfinder::world_region_state {

// P59: 测试和运行时最小 store
class InMemoryWorldRegionStateStore : public IWorldRegionStateStore {
public:
    InMemoryWorldRegionStateStore() = default;

    foundation::Result<void> put(const WorldRegionSnapshot& snapshot) override;
    foundation::Result<const WorldRegionSnapshot*> find(
        const world_generation::WorldRegionKey& key) const override;
    bool has(const world_generation::WorldRegionKey& key) const override;
    foundation::Result<void> remove(const world_generation::WorldRegionKey& key) override;
    std::vector<world_generation::WorldRegionKey> keys() const override;
    std::size_t count() const override;
    void clear() override;

private:
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<WorldRegionSnapshot>> snapshots_;
    std::map<std::string, world_generation::WorldRegionKey> keys_;

    static std::string makeKey(const world_generation::WorldRegionKey& region_key);
};

} // namespace pathfinder::world_region_state
