#include "pathfinder/world_generation/world_region_math.h"
#include <algorithm>
#include <set>

namespace pathfinder::world_generation {

int WorldRegionMath::floorDiv(int value, int divisor) {
    if (divisor <= 0) return 0;
    if (value >= 0) {
        return value / divisor;
    }
    // For negative values, C++ truncates toward zero.
    // We need floor division (toward negative infinity).
    return - ((-value + divisor - 1) / divisor);
}

WorldRegionCoord WorldRegionMath::coordToRegion(int world_x, int world_y, int region_size) {
    if (region_size <= 0) {
        return WorldRegionCoord{0, 0};
    }
    int half = region_size / 2;
    int rx = floorDiv(world_x + half, region_size);
    int ry = floorDiv(world_y + half, region_size);
    return WorldRegionCoord{rx, ry};
}

world_runtime::WorldCellCoord WorldRegionMath::regionCoordToWorld(
    const WorldRegionCoord& region,
    int local_cx,
    int local_cy,
    int region_size,
    const std::string& layer_key) {
    int world_x = region.rx * region_size + local_cx;
    int world_y = region.ry * region_size + local_cy;
    return world_runtime::WorldCellCoord{world_x, world_y, layer_key};
}

std::vector<WorldRegionCoord> WorldRegionMath::regionsCoveringCoords(
    const std::vector<world_runtime::WorldCellCoord>& coords,
    int region_size) {
    std::set<WorldRegionCoord> region_set;
    for (const auto& coord : coords) {
        region_set.insert(coordToRegion(coord.x, coord.y, region_size));
    }
    return std::vector<WorldRegionCoord>(region_set.begin(), region_set.end());
}

std::vector<WorldRegionCoord> WorldRegionMath::regionsCoveringRect(
    int min_x, int max_x, int min_y, int max_y, int region_size) {
    std::set<WorldRegionCoord> region_set;
    WorldRegionCoord min_region = coordToRegion(min_x, min_y, region_size);
    WorldRegionCoord max_region = coordToRegion(max_x, max_y, region_size);
    for (int rx = min_region.rx; rx <= max_region.rx; ++rx) {
        for (int ry = min_region.ry; ry <= max_region.ry; ++ry) {
            region_set.insert(WorldRegionCoord{rx, ry});
        }
    }
    return std::vector<WorldRegionCoord>(region_set.begin(), region_set.end());
}

std::vector<world_runtime::WorldCellCoord> WorldRegionMath::cellsInRegion(
    const WorldRegionCoord& region,
    int region_size,
    const std::string& layer_key) {
    std::vector<world_runtime::WorldCellCoord> result;
    int half = region_size / 2;
    int min_c = -half;
    int max_c = half - 1;
    if (region_size % 2 != 0) {
        max_c = half;
    }
    for (int cx = min_c; cx <= max_c; ++cx) {
        for (int cy = min_c; cy <= max_c; ++cy) {
            result.push_back(regionCoordToWorld(region, cx, cy, region_size, layer_key));
        }
    }
    return result;
}

} // namespace pathfinder::world_generation
