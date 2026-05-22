#include "pathfinder/world_region_state/in_memory_world_region_state_store.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

std::string InMemoryWorldRegionStateStore::makeKey(const world_generation::WorldRegionKey& region_key) {
    return region_key.toString();
}

Result<void> InMemoryWorldRegionStateStore::put(const WorldRegionSnapshot& snapshot) {
    auto valid = snapshot.validateBasic();
    if (valid.is_error()) {
        return valid;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = makeKey(snapshot.header.region_key);
    auto copy = std::make_unique<WorldRegionSnapshot>(snapshot);
    snapshots_[key] = std::move(copy);
    keys_[key] = snapshot.header.region_key;
    return Result<void>::ok();
}

Result<const WorldRegionSnapshot*> InMemoryWorldRegionStateStore::find(
    const world_generation::WorldRegionKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = snapshots_.find(makeKey(key));
    if (it == snapshots_.end()) {
        return Result<const WorldRegionSnapshot*>::fail(
            makeError(ErrorCode::state_snapshot_missing, "snapshot_not_found",
                      "No snapshot found for region: " + makeKey(key)));
    }
    return Result<const WorldRegionSnapshot*>::ok(it->second.get());
}

bool InMemoryWorldRegionStateStore::has(const world_generation::WorldRegionKey& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_.find(makeKey(key)) != snapshots_.end();
}

Result<void> InMemoryWorldRegionStateStore::remove(const world_generation::WorldRegionKey& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string store_key = makeKey(key);
    snapshots_.erase(store_key);
    keys_.erase(store_key);
    return Result<void>::ok();
}

std::vector<world_generation::WorldRegionKey> InMemoryWorldRegionStateStore::keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<world_generation::WorldRegionKey> result;
    result.reserve(keys_.size());
    for (const auto& [_, key] : keys_) {
        result.push_back(key);
    }
    return result;
}

std::size_t InMemoryWorldRegionStateStore::count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_.size();
}

void InMemoryWorldRegionStateStore::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshots_.clear();
    keys_.clear();
}

} // namespace pathfinder::world_region_state
