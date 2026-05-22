#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include <vector>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainConnectivityRepairer - deterministic repair of unpassable terrain
// ---------------------------------------------------------------------------

class TerrainConnectivityRepairer {
public:
    static TerrainGenerationDiagnostics repair(
        std::vector<GeneratedCellDraft>& cells,
        const world_runtime::WorldCellCoord& spawn_coord,
        const TerrainConnectivityPolicy& policy);
};

} // namespace pathfinder::world_generation
