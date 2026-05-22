#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/world_generation/world_region_math.h"

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;

MoveTargetRegionGuard::MoveTargetRegionGuard(WorldRegionEnsureService& ensure_service)
    : ensure_service_(ensure_service) {
}

Result<WorldRegionEnsureResult> MoveTargetRegionGuard::ensureMoveTarget(
    const std::string& /*actor_key*/,
    const world_runtime::WorldCellCoord& target_coord,
    const std::string& world_id,
    uint64_t world_seed,
    const std::string& generator_version,
    const std::string& content_version,
    const std::string& worldgen_profile_key,
    const std::string& layer_key,
    int region_size) {

    WorldRegionEnsureRequest request;
    request.world_id = world_id;
    request.world_seed = world_seed;
    request.generator_version = generator_version;
    request.content_version = content_version;
    request.worldgen_profile_key = worldgen_profile_key;
    request.layer_key = layer_key;
    request.region_size = region_size;
    request.ensure_kind = WorldRegionEnsureKind::MoveTarget;
    request.coverage_mode = WorldRegionCoverageMode::ExactCoords;
    request.target_coords.push_back(target_coord);
    request.max_regions_to_generate = 1;
    request.allow_generate = true;

    return ensure_service_.ensureRegions(request);
}

} // namespace pathfinder::world_generation
