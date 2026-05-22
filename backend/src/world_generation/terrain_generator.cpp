#include "pathfinder/world_generation/terrain_generator.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/world_generation/terrain_noise_field_builder.h"
#include "pathfinder/world_generation/terrain_classifier.h"
#include "pathfinder/world_generation/terrain_connectivity_analyzer.h"
#include "pathfinder/world_generation/terrain_connectivity_repairer.h"
#include <cassert>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Terrain selection (legacy weighted random)
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
// Legacy weighted random generation
// ---------------------------------------------------------------------------

static TerrainGenerator::Result generateWeightedRandom(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    WorldGenerationRng& rng) {

    TerrainGenerator::Result result;

    int min_c = WorldRegionMath::regionMinLocalCoord(request.region_size);
    int max_c = WorldRegionMath::regionMaxLocalCoord(request.region_size);

    WorldRegionKey region_key{request.world_id, profile.default_layer, request.region_coord.rx, request.region_coord.ry, request.region_size};
    std::string region_id = region_key.regionRuntimeId();

    for (int cx = min_c; cx <= max_c; ++cx) {
        for (int cy = min_c; cy <= max_c; ++cy) {
            auto coord = WorldRegionMath::regionCoordToWorld(request.region_coord, cx, cy, request.region_size, profile.default_layer);
            GeneratedCellDraft cell;
            cell.coord = coord;
            cell.region_id = region_id;

            int roll = 0;
            cell.terrain_key = TerrainGenerator::pickTerrain(rng, profile.terrain_weights, roll);
            cell.blocks_movement = TerrainGenerator::blocksMovement(cell.terrain_key);
            cell.movement_cost = TerrainGenerator::movementCost(cell.terrain_key);
            cell.biome_kind = TerrainGenerator::biomeKindFromTerrain(cell.terrain_key);

            result.trace_roll_keys.push_back("terrain_" + coord.cellId() + "_roll_" + std::to_string(roll));
            result.cells.push_back(std::move(cell));
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Noise field generation (P57)
// ---------------------------------------------------------------------------

static TerrainGenerator::Result generateNoiseField(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile) {

    TerrainGenerator::Result result;

    // Build noise field
    auto noise_field = TerrainNoiseFieldBuilder::build(request, profile);

    WorldRegionKey region_key{request.world_id, profile.default_layer, request.region_coord.rx, request.region_coord.ry, request.region_size};
    std::string region_id = region_key.regionRuntimeId();

    for (const auto& sample : noise_field.samples) {
        auto cell = TerrainClassifier::classify(request, profile, sample, region_id);
        result.cells.push_back(std::move(cell));
        result.trace_roll_keys.push_back("terrain_noise_" + sample.layer_key + "_" +
                                          std::to_string(sample.x) + "_" + std::to_string(sample.y));
    }

    // Connectivity analysis and repair
    world_runtime::WorldCellCoord spawn_coord{
        request.region_coord.rx * request.region_size,
        request.region_coord.ry * request.region_size,
        profile.default_layer
    };

    auto pre_diag = TerrainConnectivityAnalyzer::analyze(result.cells, spawn_coord, profile.connectivity_policy);
    if (pre_diag.walkable_ratio < profile.connectivity_policy.min_walkable_ratio ||
        pre_diag.blocked_ratio_in_spawn_radius > profile.connectivity_policy.max_blocked_ratio_in_spawn_radius ||
        pre_diag.reachable_from_spawn < profile.connectivity_policy.min_reachable_cells_from_spawn ||
        !pre_diag.spawn_cell_walkable) {

        if (pre_diag.walkable_ratio < profile.connectivity_policy.min_walkable_ratio) {
            result.trace_roll_keys.push_back("worldgen_walkable_ratio_low_before_repair");
        }

        auto repair_diag = TerrainConnectivityRepairer::repair(
            result.cells, spawn_coord, profile.connectivity_policy);
        result.diagnostics = repair_diag;

        for (const auto& warning : repair_diag.warning_keys) {
            result.trace_roll_keys.push_back(warning);
        }
    } else {
        result.diagnostics = pre_diag;
    }

    return result;
}

// ---------------------------------------------------------------------------
// Main generation
// ---------------------------------------------------------------------------

TerrainGenerator::Result TerrainGenerator::generate(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    WorldGenerationRng& rng) {

    switch (profile.terrain_generation_mode) {
        case TerrainGenerationMode::WeightedRandom:
            return generateWeightedRandom(request, profile, rng);
        case TerrainGenerationMode::NoiseField:
            return generateNoiseField(request, profile);
        default:
            // Fallback to noise field
            return generateNoiseField(request, profile);
    }
}

} // namespace pathfinder::world_generation
