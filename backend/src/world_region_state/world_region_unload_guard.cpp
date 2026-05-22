#include "pathfinder/world_region_state/world_region_unload_guard.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_generation::WorldRegionMath;

WorldRegionUnloadGuard::WorldRegionUnloadGuard(world_runtime::IWorldRuntime& world_runtime)
    : world_runtime_(world_runtime) {
}

bool WorldRegionUnloadGuard::isActorInRegion(
    const std::string& actor_key,
    const world_generation::WorldRegionKey& region_key) {
    auto actor_res = world_runtime_.findActor(actor_key);
    if (actor_res.is_error()) return false;
    const auto* actor = actor_res.value();
    auto actor_region = WorldRegionMath::coordToRegion(
        actor->coord.x, actor->coord.y, region_key.region_size);
    return actor_region.rx == region_key.rx && actor_region.ry == region_key.ry;
}

WorldRegionUnloadGuard::GuardCheckResult WorldRegionUnloadGuard::checkCanUnload(
    const world_generation::WorldRegionKey& region_key,
    const std::set<std::string>& active_command_region_ids) {
    GuardCheckResult result;
    result.can_unload = true;

    std::string region_id = region_key.toString();

    // Check: player actor in region
    auto player_res = world_runtime_.findActor("player");
    if (player_res.is_ok()) {
        if (isActorInRegion("player", region_key)) {
            result.can_unload = false;
            result.blocking_reason_keys.push_back("player_in_region");
        }
    }

    // Check: active command touches region
    if (active_command_region_ids.find(region_id) != active_command_region_ids.end()) {
        result.can_unload = false;
        result.blocking_reason_keys.push_back("active_command_in_region");
    }

    // Check: unsupported ownership state
    auto ownership_check = checkNoComplexOwnership(region_key);
    if (!ownership_check.can_unload) {
        result.can_unload = false;
        result.blocking_reason_keys.insert(
            result.blocking_reason_keys.end(),
            ownership_check.blocking_reason_keys.begin(),
            ownership_check.blocking_reason_keys.end());
    }

    return result;
}

WorldRegionUnloadGuard::GuardCheckResult WorldRegionUnloadGuard::checkNoComplexOwnership(
    const world_generation::WorldRegionKey& region_key) {
    GuardCheckResult result;
    result.can_unload = true;

    // Get all cells in this region
    auto cells = WorldRegionMath::cellsInRegion(
        world_generation::WorldRegionCoord{region_key.rx, region_key.ry},
        region_key.region_size, region_key.layer_key);

    for (const auto& coord : cells) {
        auto cell_res = world_runtime_.findCell(coord);
        if (cell_res.is_error()) continue;
        const auto* cell = cell_res.value();

        for (const auto& entity_id : cell->entity_ids) {
            auto entity_res = world_runtime_.findEntity(entity_id);
            if (entity_res.is_error()) continue;
            const auto* entity = entity_res.value();

            // OnMap entities are fine
            if (entity->location_kind == world_runtime::WorldEntityLocationKind::OnMap) {
                continue;
            }

            // Any non-OnMap entity on a cell is suspicious
            if (entity->location_kind == world_runtime::WorldEntityLocationKind::InInventory ||
                entity->location_kind == world_runtime::WorldEntityLocationKind::InContainer ||
                entity->location_kind == world_runtime::WorldEntityLocationKind::Equipped) {
                result.can_unload = false;
                result.blocking_reason_keys.push_back(
                    "unsupported_ownership:" + entity_id + ":" + world_runtime::toString(entity->location_kind));
            }
        }
    }

    return result;
}

} // namespace pathfinder::world_region_state
