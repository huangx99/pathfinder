#pragma once

#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/foundation/result.h"
#include <map>
#include <set>
#include <string>

namespace pathfinder::world_map_interaction {

// ---------------------------------------------------------------------------
// RegionActivityWindowService
// ---------------------------------------------------------------------------
// P60: Computes which regions should stay active, be sealed, or detached
// based on actor position, viewport, vision radius, danger, and agent activity.
// ---------------------------------------------------------------------------

class RegionActivityWindowService {
public:
    struct Config {
        int vision_radius_cells = 18;          // actor vision radius in cells
        int preload_margin_regions = 1;        // extra region rings to keep active
        int seal_delay_ticks = 10;             // ticks before a left region becomes seal candidate
        int detach_delay_ticks = 3;            // ticks after seal before detach candidate
        int recently_touched_window_ticks = 10; // how long to delay seal after last interaction
    };

    RegionActivityWindowService(
        const Config& config,
        world_runtime::IWorldRuntime& world_runtime);

    // Compute the activity window for the given actor.
    // Call this after any position change, viewport change, or world tick advance.
    pathfinder::foundation::Result<RegionActivityWindow> computeWindow(
        const std::string& actor_key,
        const std::string& layer_key,
        uint64_t current_tick);

    // Record that a region was recently interacted with (harvest, pickup, drop, etc.)
    void touchRegion(const world_generation::WorldRegionKey& region_key, uint64_t current_tick);

    // Record that the actor's current region was recently interacted with.
    pathfinder::foundation::Result<void> touchActorRegion(
        const std::string& actor_key,
        const std::string& layer_key,
        uint64_t current_tick);

    // Mark a region as successfully sealed (so it can become detach candidate after delay)
    void markSealed(const world_generation::WorldRegionKey& region_key, uint64_t current_tick);

    // Clear tracking data (e.g., on world reset)
    void clearTracking();

    // P60 testing: allow runtime config override for integration tests.
    void setConfig(const Config& config) { config_ = config; }

private:
    Config config_;
    world_runtime::IWorldRuntime& world_runtime_;


    // Region key string -> last touched tick
    std::map<std::string, uint64_t> region_last_touched_;
    // Region key string -> first tick when it left active window
    std::map<std::string, uint64_t> region_left_active_since_;
    // Region key string -> seal success tick
    std::map<std::string, uint64_t> region_sealed_at_;

    std::string makeKey(const world_generation::WorldRegionKey& key) const;

    std::vector<world_generation::WorldRegionKey> computeKeepActiveRegions(
        const world_runtime::WorldActorRuntime& actor,
        const std::string& layer_key);

    std::vector<world_generation::WorldRegionKey> computePrewarmRegions(
        const std::vector<world_generation::WorldRegionKey>& keep_active,
        const std::string& layer_key);

    std::vector<world_generation::WorldRegionKey> findCurrentlyActiveRegions(
        const std::string& layer_key);

    std::set<std::string> collectBlockedRegionIds(
        const std::string& layer_key);
};

} // namespace pathfinder::world_map_interaction
