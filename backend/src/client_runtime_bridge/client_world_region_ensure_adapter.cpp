#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <set>
#include <algorithm>

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_region_state;

namespace {

std::vector<WorldRegionCoord> computeDesiredRegionsForAdapter(
    const WorldRegionEnsureRequest& request) {
    std::set<WorldRegionCoord> regions;

    switch (request.coverage_mode) {
        case WorldRegionCoverageMode::ExactCoords: {
            auto covered = WorldRegionMath::regionsCoveringCoords(request.target_coords, request.region_size);
            regions.insert(covered.begin(), covered.end());
            break;
        }
        case WorldRegionCoverageMode::ActorVisionWindow: {
            if (request.center_coord) {
                int r = request.radius_cells;
                auto covered = WorldRegionMath::regionsCoveringRect(
                    request.center_coord->x - r,
                    request.center_coord->x + r,
                    request.center_coord->y - r,
                    request.center_coord->y + r,
                    request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::ActorNeighborMoves: {
            if (request.center_coord) {
                std::vector<WorldCellCoord> neighbors;
                neighbors.push_back(WorldCellCoord{request.center_coord->x, request.center_coord->y - 1, request.layer_key});
                neighbors.push_back(WorldCellCoord{request.center_coord->x, request.center_coord->y + 1, request.layer_key});
                neighbors.push_back(WorldCellCoord{request.center_coord->x - 1, request.center_coord->y, request.layer_key});
                neighbors.push_back(WorldCellCoord{request.center_coord->x + 1, request.center_coord->y, request.layer_key});
                auto covered = WorldRegionMath::regionsCoveringCoords(neighbors, request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::RectWindow: {
            if (request.center_coord) {
                int r = request.radius_cells;
                auto covered = WorldRegionMath::regionsCoveringRect(
                    request.center_coord->x - r,
                    request.center_coord->x + r,
                    request.center_coord->y - r,
                    request.center_coord->y + r,
                    request.region_size);
                regions.insert(covered.begin(), covered.end());
            }
            break;
        }
        case WorldRegionCoverageMode::RegionList:
        case WorldRegionCoverageMode::TestOnly: {
            regions.insert(request.explicit_regions.begin(), request.explicit_regions.end());
            break;
        }
        default:
            break;
    }

    // Apply preload margin
    if (request.preload_margin_regions > 0 && request.center_coord) {
        auto center_region = WorldRegionMath::coordToRegion(
            request.center_coord->x, request.center_coord->y, request.region_size);
        int margin = request.preload_margin_regions;
        for (int rx = center_region.rx - margin; rx <= center_region.rx + margin; ++rx) {
            for (int ry = center_region.ry - margin; ry <= center_region.ry + margin; ++ry) {
                regions.insert(WorldRegionCoord{rx, ry});
            }
        }
    }

    return std::vector<WorldRegionCoord>(regions.begin(), regions.end());
}

} // anonymous namespace

ClientWorldRegionEnsureAdapter::ClientWorldRegionEnsureAdapter(
    WorldRegionEnsureService& ensure_service,
    IWorldRuntime& world_runtime,
    WorldRegionLifecycleService* lifecycle_service)
    : ensure_service_(ensure_service)
    , world_runtime_(world_runtime)
    , lifecycle_service_(lifecycle_service) {
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureWithRequest(
    const std::string& actor_key,
    const std::string& layer_key,
    WorldRegionCoverageMode mode,
    int radius_cells,
    int max_regions,
    WorldRegionEnsureKind ensure_kind) {

    auto actor_res = world_runtime_.findActor(actor_key);
    if (actor_res.is_error()) {
        return Result<WorldRegionEnsureResult>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + actor_key));
    }

    const auto* actor = actor_res.value();
    return ensureWithCenter(actor_key, layer_key, actor->coord, mode, radius_cells, max_regions, ensure_kind);
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureWithCenter(
    const std::string& actor_key,
    const std::string& layer_key,
    const WorldCellCoord& center_coord,
    WorldRegionCoverageMode mode,
    int radius_cells,
    int max_regions,
    WorldRegionEnsureKind ensure_kind) {

    auto actor_res = world_runtime_.findActor(actor_key);
    if (actor_res.is_error()) {
        return Result<WorldRegionEnsureResult>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + actor_key));
    }

    WorldRegionEnsureRequest request;
    request.world_id = world_runtime_.worldId();
    request.world_seed = world_runtime_.worldSeed();
    request.generator_version = "1.0.0";
    request.content_version = "1.0.0";
    request.worldgen_profile_key = "first_world";
    request.layer_key = layer_key.empty() ? "surface" : layer_key;
    request.region_size = world_runtime_.regionSize();
    request.ensure_kind = ensure_kind;
    request.coverage_mode = mode;
    request.center_coord = center_coord;
    request.radius_cells = radius_cells;
    request.max_regions_to_generate = max_regions;
    request.allow_generate = true;

    // P59: If lifecycle service is available, use it for active -> restore -> generate
    if (lifecycle_service_ != nullptr) {
        // Compute desired regions
        auto desired_regions = computeDesiredRegionsForAdapter(request);
        std::sort(desired_regions.begin(), desired_regions.end());
        auto last = std::unique(desired_regions.begin(), desired_regions.end());
        desired_regions.erase(last, desired_regions.end());

        WorldRegionEnsureResult result;
        result.ok = true;
        result.request_id = request.world_id + ":" + toString(request.ensure_kind);
        result.ensure_kind = request.ensure_kind;
        result.coverage_mode = request.coverage_mode;

        WorldRegionSnapshotBuildContext build_context;
        build_context.world_id = request.world_id;
        build_context.world_seed = request.world_seed;
        build_context.generator_version = request.generator_version;
        build_context.content_version = request.content_version;
        build_context.worldgen_profile_key = request.worldgen_profile_key;

        for (const auto& region : desired_regions) {
            world_generation::WorldRegionKey region_key;
            region_key.world_id = request.world_id;
            region_key.layer_key = request.layer_key;
            region_key.rx = region.rx;
            region_key.ry = region.ry;
            region_key.region_size = request.region_size;

            auto avail_res = lifecycle_service_->ensureAvailable(region_key, build_context);
            WorldRegionEnsureItemResult item;
            item.region_key = region_key;

            if (avail_res.is_ok() && avail_res.value().available) {
                item.status = WorldRegionEnsureStatus::AlreadyAvailable;
                const auto& avail = avail_res.value();
                if (avail.source == WorldRegionAvailabilitySource::GeneratedBaseline) {
                    item.status = WorldRegionEnsureStatus::GeneratedAndApplied;
                    ++result.generated_region_count;
                } else if (avail.source == WorldRegionAvailabilitySource::RestoredSnapshot) {
                    ++result.generated_region_count; // Count as "made available"
                } else {
                    ++result.already_available_count;
                }
                item.changed_cell_ids = avail.changed_cell_ids;
                item.changed_entity_ids = avail.changed_entity_ids;
                item.warning_keys = avail.warning_keys;
            } else {
                result.ok = false;
                item.status = WorldRegionEnsureStatus::GenerationFailed;
                if (avail_res.is_ok()) {
                    item.failure_reason_key = "lifecycle_ensure_failed";
                    for (const auto& key : avail_res.value().failure_reason_keys) {
                        item.failure_reason_key = "lifecycle_ensure_failed";
                        result.failure_reason_keys.push_back(key);
                    }
                } else {
                    item.failure_reason_key = "lifecycle_ensure_error";
                    for (const auto& err : avail_res.errors()) {
                        item.failure_reason_key = "lifecycle_ensure_error";
                        result.failure_reason_keys.push_back(err.message_key);
                    }
                }
            }

            result.item_results.push_back(std::move(item));
            result.changed_cell_ids.insert(result.changed_cell_ids.end(), item.changed_cell_ids.begin(), item.changed_cell_ids.end());
            result.changed_entity_ids.insert(result.changed_entity_ids.end(), item.changed_entity_ids.begin(), item.changed_entity_ids.end());
        }

        // Deduplicate
        std::sort(result.changed_cell_ids.begin(), result.changed_cell_ids.end());
        result.changed_cell_ids.erase(std::unique(result.changed_cell_ids.begin(), result.changed_cell_ids.end()), result.changed_cell_ids.end());
        std::sort(result.changed_entity_ids.begin(), result.changed_entity_ids.end());
        result.changed_entity_ids.erase(std::unique(result.changed_entity_ids.begin(), result.changed_entity_ids.end()), result.changed_entity_ids.end());

        auto refresh_result = world_runtime_.refreshActorExploration(actor_key);
        if (refresh_result.is_error()) {
            result.warning_keys.push_back("actor_exploration_refresh_failed");
        }
        return Result<WorldRegionEnsureResult>::ok(std::move(result));
    }

    auto ensure_result = ensure_service_.ensureRegions(request);
    if (ensure_result.is_ok()) {
        auto refresh_result = world_runtime_.refreshActorExploration(actor_key);
        if (refresh_result.is_error()) {
            ensure_result.value().warning_keys.push_back("actor_exploration_refresh_failed");
        }
    }
    return ensure_result;
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureForBootstrap(
    const std::string& actor_key,
    const std::string& layer_key) {
    // Bootstrap: ensure actor vision window + neighbor moves
    // Use actor vision radius from runtime
    auto actor_res = world_runtime_.findActor(actor_key);
    int vision_radius = 3;
    if (actor_res.is_ok()) {
        vision_radius = actor_res.value()->vision_radius;
    }
    return ensureWithRequest(actor_key, layer_key, WorldRegionCoverageMode::ActorVisionWindow,
                             vision_radius, 25, WorldRegionEnsureKind::BootstrapWindow);
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureForRefresh(
    const std::string& actor_key,
    const std::string& layer_key,
    int vision_radius) {
    // If caller passes 0 or negative, use actor's actual vision radius from runtime.
    if (vision_radius <= 0) {
        auto actor_res = world_runtime_.findActor(actor_key);
        if (actor_res.is_ok()) {
            vision_radius = actor_res.value()->vision_radius;
        } else {
            vision_radius = 3; // fallback
        }
    }
    return ensureWithRequest(actor_key, layer_key, WorldRegionCoverageMode::ActorVisionWindow,
                             vision_radius, 25, WorldRegionEnsureKind::RefreshProjectionWindow);
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureForAvailableCommands(
    const std::string& actor_key,
    const std::string& layer_key) {
    return ensureWithRequest(actor_key, layer_key, WorldRegionCoverageMode::ActorNeighborMoves,
                             1, 4, WorldRegionEnsureKind::AvailableCommandWindow);
}

Result<WorldRegionEnsureResult> ClientWorldRegionEnsureAdapter::ensureForTargetVision(
    const std::string& actor_key,
    const std::string& layer_key,
    const WorldCellCoord& target_coord) {
    auto actor_res = world_runtime_.findActor(actor_key);
    int vision_radius = 3;
    if (actor_res.is_ok()) {
        vision_radius = actor_res.value()->vision_radius;
    }
    return ensureWithCenter(actor_key, layer_key, target_coord, WorldRegionCoverageMode::ActorVisionWindow,
                            vision_radius, 25, WorldRegionEnsureKind::RefreshProjectionWindow);
}

} // namespace pathfinder::client_runtime_bridge
