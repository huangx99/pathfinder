#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include <vector>

namespace pathfinder::world_generation {

class WorldRegionMath {
public:
    static int floorDiv(int value, int divisor);

    static WorldRegionCoord coordToRegion(
        int world_x,
        int world_y,
        int region_size);

    static world_runtime::WorldCellCoord regionCoordToWorld(
        const WorldRegionCoord& region,
        int local_cx,
        int local_cy,
        int region_size,
        const std::string& layer_key);

    static std::vector<WorldRegionCoord> regionsCoveringCoords(
        const std::vector<world_runtime::WorldCellCoord>& coords,
        int region_size);

    static std::vector<WorldRegionCoord> regionsCoveringRect(
        int min_x,
        int max_x,
        int min_y,
        int max_y,
        int region_size);

    static std::vector<world_runtime::WorldCellCoord> cellsInRegion(
        const WorldRegionCoord& region,
        int region_size,
        const std::string& layer_key);
};

} // namespace pathfinder::world_generation
