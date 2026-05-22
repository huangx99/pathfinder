#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_map_interaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_generation::WorldRegionMath;
using pathfinder::world_generation::WorldRegionCoord;
using pathfinder::world_generation::WorldRegionKey;
using pathfinder::world_runtime::WorldActorRuntime;
using pathfinder::world_runtime::WorldCellCoord;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

RegionActivityWindowService::RegionActivityWindowService(
    const Config& config,
    world_runtime::IWorldRuntime& world_runtime)
    : config_(config)
    , world_runtime_(world_runtime) {}

// ---------------------------------------------------------------------------
// Tracking helpers
// ---------------------------------------------------------------------------

std::string RegionActivityWindowService::makeKey(const WorldRegionKey& key) const {
    return key.regionRuntimeId();
}

void RegionActivityWindowService::touchRegion(const WorldRegionKey& region_key, uint64_t current_tick) {
    const std::string key = makeKey(region_key);
    region_last_touched_[key] = current_tick;
    region_left_active_since_.erase(key);
}

Result<void> RegionActivityWindowService::touchActorRegion(
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t current_tick) {

    auto actor_res = world_runtime_.findActor(actor_key);
    if (actor_res.is_error()) {
        return Result<void>::fail(actor_res.errors());
    }
    const auto* actor = actor_res.value();
    if (!actor) {
        return Result<void>::fail(makeError(ErrorCode::id_not_found,
            "Actor is null: " + actor_key));
    }

    int region_size = world_runtime_.regionSize();
    if (region_size <= 0) region_size = 16;
    WorldRegionCoord rc = WorldRegionMath::coordToRegion(actor->coord.x, actor->coord.y, region_size);

    WorldRegionKey key;
    key.world_id = world_runtime_.worldId();
    key.layer_key = layer_key.empty() ? actor->coord.layer_key : layer_key;
    key.rx = rc.rx;
    key.ry = rc.ry;
    key.region_size = region_size;
    touchRegion(key, current_tick);
    return Result<void>::ok();
}

void RegionActivityWindowService::markSealed(const WorldRegionKey& region_key, uint64_t current_tick) {
    const std::string key = makeKey(region_key);
    region_sealed_at_[key] = current_tick;
    region_left_active_since_.erase(key);
}

void RegionActivityWindowService::clearTracking() {
    region_last_touched_.clear();
    region_left_active_since_.clear();
    region_sealed_at_.clear();
}

// ---------------------------------------------------------------------------
// Compute window
// ---------------------------------------------------------------------------

Result<RegionActivityWindow> RegionActivityWindowService::computeWindow(
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t current_tick) {

    auto actor_res = world_runtime_.findActor(actor_key);
    if (actor_res.is_error()) {
        return Result<RegionActivityWindow>::fail(makeError(ErrorCode::id_not_found,
            "Actor not found: " + actor_key));
    }
    const auto* actor = actor_res.value();
    if (!actor) {
        return Result<RegionActivityWindow>::fail(makeError(ErrorCode::id_not_found,
            "Actor is null: " + actor_key));
    }

    RegionActivityWindow window;
    window.world_id = world_runtime_.worldId();
    window.layer_key = layer_key;
    window.center_actor_key = actor_key;
    window.center_coord = actor->coord;
    window.vision_radius = config_.vision_radius_cells;
    window.preload_margin_regions = config_.preload_margin_regions;
    window.computed_tick = current_tick;

    int region_size = world_runtime_.regionSize();
    if (region_size <= 0) region_size = 16;

    // Viewport rect based on vision radius
    window.viewport_min_x = actor->coord.x - config_.vision_radius_cells;
    window.viewport_max_x = actor->coord.x + config_.vision_radius_cells;
    window.viewport_min_y = actor->coord.y - config_.vision_radius_cells;
    window.viewport_max_y = actor->coord.y + config_.vision_radius_cells;

    // 1. Compute keep-active regions
    window.keep_active_regions = computeKeepActiveRegions(*actor, layer_key);

    // 2. Compute prewarm regions (outer ring beyond keep_active)
    window.prewarm_regions = computePrewarmRegions(window.keep_active_regions, layer_key);

    // 3. Collect blocked region ids (actors, agents, danger)
    std::set<std::string> blocked_ids = collectBlockedRegionIds(layer_key);

    // 4. Find currently active regions in runtime
    std::vector<WorldRegionKey> currently_active = findCurrentlyActiveRegions(layer_key);

    // 5. Determine seal candidates
    std::set<std::string> keep_active_set;
    for (const auto& r : window.keep_active_regions) {
        const std::string key = makeKey(r);
        keep_active_set.insert(key);
        region_left_active_since_.erase(key);
    }

    for (const auto& active : currently_active) {
        std::string key = makeKey(active);
        if (keep_active_set.count(key)) continue;
        if (blocked_ids.count(key)) {
            window.blocked_regions.push_back(active);
            continue;
        }

        // Check recently touched delay
        auto touched_it = region_last_touched_.find(key);
        if (touched_it != region_last_touched_.end()) {
            uint64_t elapsed = current_tick - touched_it->second;
            if (elapsed < static_cast<uint64_t>(config_.recently_touched_window_ticks)) {
                window.blocked_regions.push_back(active);
                continue;
            }
        }

        auto sealed_it = region_sealed_at_.find(key);
        if (sealed_it != region_sealed_at_.end()) {
            uint64_t elapsed = current_tick - sealed_it->second;
            if (elapsed >= static_cast<uint64_t>(config_.detach_delay_ticks)) {
                window.detach_candidate_regions.push_back(active);
            } else {
                window.blocked_regions.push_back(active);
            }
            continue;
        }

        auto left_it = region_left_active_since_.find(key);
        if (left_it == region_left_active_since_.end()) {
            region_left_active_since_[key] = current_tick;
            if (config_.seal_delay_ticks > 0) {
                window.blocked_regions.push_back(active);
                continue;
            }
            window.seal_candidate_regions.push_back(active);
            continue;
        }

        uint64_t elapsed_since_left = current_tick - left_it->second;
        if (elapsed_since_left >= static_cast<uint64_t>(config_.seal_delay_ticks)) {
            window.seal_candidate_regions.push_back(active);
        } else {
            window.blocked_regions.push_back(active);
        }
    }

    return Result<RegionActivityWindow>::ok(std::move(window));
}

// ---------------------------------------------------------------------------
// Keep active regions = actor region + viewport regions + preload margin
// ---------------------------------------------------------------------------

std::vector<WorldRegionKey> RegionActivityWindowService::computeKeepActiveRegions(
    const WorldActorRuntime& actor,
    const std::string& layer_key) {

    std::vector<WorldRegionKey> result;
    int region_size = world_runtime_.regionSize();
    if (region_size <= 0) region_size = 16;

    // Actor's own region
    WorldRegionCoord actor_region = WorldRegionMath::coordToRegion(
        actor.coord.x, actor.coord.y, region_size);

    // Determine rect covering vision + preload margin
    int margin_cells = config_.preload_margin_regions * region_size;
    int min_x = actor.coord.x - config_.vision_radius_cells - margin_cells;
    int max_x = actor.coord.x + config_.vision_radius_cells + margin_cells;
    int min_y = actor.coord.y - config_.vision_radius_cells - margin_cells;
    int max_y = actor.coord.y + config_.vision_radius_cells + margin_cells;

    std::vector<WorldRegionCoord> coords = WorldRegionMath::regionsCoveringRect(
        min_x, max_x, min_y, max_y, region_size);

    for (const auto& rc : coords) {
        WorldRegionKey key;
        key.world_id = world_runtime_.worldId();
        key.layer_key = layer_key;
        key.rx = rc.rx;
        key.ry = rc.ry;
        key.region_size = region_size;
        result.push_back(std::move(key));
    }

    return result;
}

// ---------------------------------------------------------------------------
// Prewarm regions = one extra ring beyond keep_active (for future expansion)
// ---------------------------------------------------------------------------

std::vector<WorldRegionKey> RegionActivityWindowService::computePrewarmRegions(
    const std::vector<WorldRegionKey>& keep_active,
    const std::string& layer_key) {

    // P60: Prewarm ring = one extra region margin beyond keep_active.
    // This ensures smoother client experience when the player moves toward the edge.
    if (keep_active.empty()) {
        return {};
    }

    int region_size = keep_active[0].region_size;
    if (region_size <= 0) region_size = 16;

    // Find bounding box of keep_active regions
    int min_rx = keep_active[0].rx;
    int max_rx = keep_active[0].rx;
    int min_ry = keep_active[0].ry;
    int max_ry = keep_active[0].ry;
    for (const auto& r : keep_active) {
        min_rx = std::min(min_rx, r.rx);
        max_rx = std::max(max_rx, r.rx);
        min_ry = std::min(min_ry, r.ry);
        max_ry = std::max(max_ry, r.ry);
    }

    // Expand by one region in each direction
    std::set<std::string> keep_active_set;
    for (const auto& r : keep_active) {
        keep_active_set.insert(makeKey(r));
    }

    std::vector<WorldRegionKey> result;
    for (int rx = min_rx - 1; rx <= max_rx + 1; ++rx) {
        for (int ry = min_ry - 1; ry <= max_ry + 1; ++ry) {
            WorldRegionKey key;
            key.world_id = keep_active[0].world_id;
            key.layer_key = layer_key;
            key.rx = rx;
            key.ry = ry;
            key.region_size = region_size;
            if (!keep_active_set.count(makeKey(key))) {
                result.push_back(std::move(key));
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Find all currently active regions in runtime
// ---------------------------------------------------------------------------

std::vector<WorldRegionKey> RegionActivityWindowService::findCurrentlyActiveRegions(
    const std::string& layer_key) {

    std::vector<WorldRegionKey> result;
    int region_size = world_runtime_.regionSize();
    if (region_size <= 0) region_size = 16;

    // Iterate through all generated regions in runtime
    auto snap_res = world_runtime_.snapshotForDebug();
    if (snap_res.is_error()) {
        return result;
    }
    const auto& snapshot = snap_res.value();

    std::set<std::string> seen_region_ids;
    for (const auto& [cell_id, cell] : snapshot.cells) {
        if (!cell.region_id.empty() && seen_region_ids.insert(cell.region_id).second) {
            // Parse region_id format: world_id:layer_key:region:rx:ry:region_size
            // For simplicity, we can also use the cell's coord to compute region
            WorldRegionCoord rc = WorldRegionMath::coordToRegion(
                cell.coord.x, cell.coord.y, region_size);
            WorldRegionKey key;
            key.world_id = world_runtime_.worldId();
            key.layer_key = layer_key;
            key.rx = rc.rx;
            key.ry = rc.ry;
            key.region_size = region_size;
            result.push_back(std::move(key));
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Collect blocked region ids
// ---------------------------------------------------------------------------

std::set<std::string> RegionActivityWindowService::collectBlockedRegionIds(
    const std::string& layer_key) {

    std::set<std::string> blocked;
    int region_size = world_runtime_.regionSize();
    if (region_size <= 0) region_size = 16;

    auto snap_res = world_runtime_.snapshotForDebug();
    if (snap_res.is_error()) {
        return blocked;
    }
    const auto& snapshot = snap_res.value();

    // Block regions containing any actor (including player and companions)
    for (const auto& [actor_key, actor] : snapshot.actors) {
        WorldRegionCoord rc = WorldRegionMath::coordToRegion(
            actor.coord.x, actor.coord.y, region_size);
        WorldRegionKey key;
        key.world_id = world_runtime_.worldId();
        key.layer_key = layer_key;
        key.rx = rc.rx;
        key.ry = rc.ry;
        key.region_size = region_size;
        blocked.insert(makeKey(key));
    }

    // TODO: P60 future - block regions with:
    //   - active AI agents (when agent execution system is integrated)
    //   - unresolved danger / combat events
    //   - ongoing multi-tick commands (e.g. long crafting, construction)
    // On-map entities (resource nodes, ground items) do NOT block seal because
    // their state is preserved in the snapshot and restored correctly.

    return blocked;
}

} // namespace pathfinder::world_map_interaction
