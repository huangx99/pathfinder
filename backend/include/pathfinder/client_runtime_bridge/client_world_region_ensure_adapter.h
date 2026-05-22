#pragma once

#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::client_runtime_bridge {

class ClientWorldRegionEnsureAdapter {
public:
    ClientWorldRegionEnsureAdapter(
        pathfinder::world_generation::WorldRegionEnsureService& ensure_service,
        pathfinder::world_runtime::IWorldRuntime& world_runtime);

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureForBootstrap(
        const std::string& actor_key,
        const std::string& layer_key);

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureForRefresh(
        const std::string& actor_key,
        const std::string& layer_key,
        int vision_radius);

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureForAvailableCommands(
        const std::string& actor_key,
        const std::string& layer_key);

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureForTargetVision(
        const std::string& actor_key,
        const std::string& layer_key,
        const pathfinder::world_runtime::WorldCellCoord& target_coord);

private:
    pathfinder::world_generation::WorldRegionEnsureService& ensure_service_;
    pathfinder::world_runtime::IWorldRuntime& world_runtime_;

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureWithRequest(
        const std::string& actor_key,
        const std::string& layer_key,
        pathfinder::world_generation::WorldRegionCoverageMode mode,
        int radius_cells,
        int max_regions,
        pathfinder::world_generation::WorldRegionEnsureKind ensure_kind);

    pathfinder::foundation::Result<pathfinder::world_generation::WorldRegionEnsureResult> ensureWithCenter(
        const std::string& actor_key,
        const std::string& layer_key,
        const pathfinder::world_runtime::WorldCellCoord& center_coord,
        pathfinder::world_generation::WorldRegionCoverageMode mode,
        int radius_cells,
        int max_regions,
        pathfinder::world_generation::WorldRegionEnsureKind ensure_kind);
};

} // namespace pathfinder::client_runtime_bridge
