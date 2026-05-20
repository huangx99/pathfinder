#include "pathfinder/world_generation/terrain_generator.h"
#include <cassert>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Terrain selection
// ---------------------------------------------------------------------------

std::string TerrainGenerator::pickTerrain(
    WorldGenerationRng& rng,
    const std::vector<TerrainWeight>& weights,
    int& out_roll) {
    if (weights.empty()) {
        out_roll = 0;
        return "plain";
    }
    int total = 0;
    for (const auto& w : weights) {
        total += w.weight;
    }
    if (total <= 0) {
        out_roll = 0;
        return "plain";
    }
    out_roll = rng.drawInt(0, total - 1);
    int cumulative = 0;
    for (const auto& w : weights) {
        cumulative += w.weight;
        if (out_roll < cumulative) {
            return w.terrain_key;
        }
    }
    return weights.empty() ? "plain" : weights.back().terrain_key;
}

bool TerrainGenerator::blocksMovement(const std::string& terrain_key) {
    if (terrain_key == "water" || terrain_key == "mountain" || terrain_key == "blocked") {
        return true;
    }
    return false;
}

int TerrainGenerator::movementCost(const std::string& terrain_key) {
    if (terrain_key == "forest") return 2;
    if (terrain_key == "water" || terrain_key == "mountain" || terrain_key == "blocked") return 99;
    return 1;
}

WorldBiomeKind TerrainGenerator::biomeKindFromTerrain(const std::string& terrain_key) {
    if (terrain_key == "plain") return WorldBiomeKind::Plains;
    if (terrain_key == "forest") return WorldBiomeKind::Forest;
    if (terrain_key == "water" || terrain_key == "water_edge") return WorldBiomeKind::WaterEdge;
    if (terrain_key == "mountain" || terrain_key == "stone_field") return WorldBiomeKind::StoneField;
    if (terrain_key == "wetland") return WorldBiomeKind::Wetland;
    if (terrain_key == "cave") return WorldBiomeKind::Cave;
    if (terrain_key == "ruins") return WorldBiomeKind::Ruins;
    if (terrain_key == "danger_nest") return WorldBiomeKind::DangerNest;
    return WorldBiomeKind::Unknown;
}

// ---------------------------------------------------------------------------
// Main generation
// ---------------------------------------------------------------------------

TerrainGenerator::Result TerrainGenerator::generate(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    WorldGenerationRng& rng) {
    Result result;

    int radius = request.region_size / 2;
    // For even sizes, center around 0 with asymmetric bounds
    int min_c = -(request.region_size / 2);
    int max_c = request.region_size / 2 - (request.region_size % 2 == 0 ? 1 : 0);
    if (request.region_size % 2 == 1) {
        min_c = -radius;
        max_c = radius;
    }

    std::string region_id = request.region_coord.regionId();

    for (int cx = min_c; cx <= max_c; ++cx) {
        for (int cy = min_c; cy <= max_c; ++cy) {
            int world_x = request.region_coord.rx * request.region_size + cx;
            int world_y = request.region_coord.ry * request.region_size + cy;
            world_runtime::WorldCellCoord coord{world_x, world_y, profile.default_layer};
            GeneratedCellDraft cell;
            cell.coord = coord;
            cell.region_id = region_id;

            int roll = 0;
            cell.terrain_key = pickTerrain(rng, profile.terrain_weights, roll);
            cell.blocks_movement = blocksMovement(cell.terrain_key);
            cell.movement_cost = movementCost(cell.terrain_key);
            cell.biome_kind = biomeKindFromTerrain(cell.terrain_key);

            result.trace_roll_keys.push_back("terrain_" + coord.cellId() + "_roll_" + std::to_string(roll));
            result.cells.push_back(std::move(cell));
        }
    }

    return result;
}

} // namespace pathfinder::world_generation
