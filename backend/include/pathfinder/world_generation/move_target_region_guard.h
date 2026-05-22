#pragma once

#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_generation {

class MoveTargetRegionGuard {
public:
    explicit MoveTargetRegionGuard(WorldRegionEnsureService& ensure_service);

    pathfinder::foundation::Result<WorldRegionEnsureResult> ensureMoveTarget(
        const std::string& actor_key,
        const world_runtime::WorldCellCoord& target_coord,
        const std::string& world_id,
        uint64_t world_seed,
        const std::string& generator_version,
        const std::string& content_version,
        const std::string& worldgen_profile_key,
        const std::string& layer_key,
        int region_size);

private:
    WorldRegionEnsureService& ensure_service_;
};

} // namespace pathfinder::world_generation
