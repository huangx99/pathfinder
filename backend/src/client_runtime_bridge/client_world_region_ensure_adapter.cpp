#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;

ClientWorldRegionEnsureAdapter::ClientWorldRegionEnsureAdapter(
    WorldRegionEnsureService& ensure_service,
    IWorldRuntime& world_runtime)
    : ensure_service_(ensure_service)
    , world_runtime_(world_runtime) {
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
