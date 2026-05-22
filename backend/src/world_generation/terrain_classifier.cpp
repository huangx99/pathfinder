#include "pathfinder/world_generation/terrain_classifier.h"
#include <algorithm>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Terrain movement rules
// ---------------------------------------------------------------------------

bool TerrainClassifier::blocksMovement(const std::string& terrain_key) {
    if (terrain_key == "blocked" || terrain_key == "mountain" || terrain_key == "deep_water") {
        return true;
    }
    return false;
}

int TerrainClassifier::movementCost(const std::string& terrain_key) {
    if (terrain_key == "forest") return 2;
    if (terrain_key == "stone_field") return 2;
    if (terrain_key == "water_edge") return 2;
    if (terrain_key == "blocked" || terrain_key == "mountain" || terrain_key == "deep_water") return 99;
    return 1;
}

WorldBiomeKind TerrainClassifier::biomeKindFromTerrain(const std::string& terrain_key) {
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
// Tag derivation
// ---------------------------------------------------------------------------

std::vector<std::string> TerrainClassifier::deriveTags(
    const std::string& terrain_key,
    bool blocks_movement,
    const TerrainNoiseSample& sample) {

    std::vector<std::string> tags;
    tags.push_back("terrain:" + terrain_key);

    if (!blocks_movement) {
        tags.push_back("walkable");
    } else {
        tags.push_back("blocked");
    }

    auto biome = biomeKindFromTerrain(terrain_key);
    if (biome != WorldBiomeKind::Unknown) {
        tags.push_back("biome:" + toString(biome));
    }

    if (sample.roughness > 0.3) {
        tags.push_back("rough");
    }
    if (sample.moisture > 0.2) {
        tags.push_back("wet");
    }
    if (sample.resource_richness > 0.3) {
        tags.push_back("resource_rich");
    }

    return tags;
}

// ---------------------------------------------------------------------------
// Classification
// ---------------------------------------------------------------------------

GeneratedCellDraft TerrainClassifier::classify(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    const TerrainNoiseSample& sample,
    const std::string& region_id) {

    GeneratedCellDraft cell;
    cell.coord = world_runtime::WorldCellCoord{sample.x, sample.y, sample.layer_key};
    cell.region_id = region_id;

    // Evaluate rules by priority
    const TerrainThresholdRule* best_rule = nullptr;
    int best_priority = -1;

    for (const auto& rule : profile.terrain_threshold_rules) {
        bool matches = true;
        if (sample.elevation < rule.min_elevation || sample.elevation > rule.max_elevation) matches = false;
        if (sample.moisture < rule.min_moisture || sample.moisture > rule.max_moisture) matches = false;
        if (sample.roughness < rule.min_roughness || sample.roughness > rule.max_roughness) matches = false;

        if (matches && rule.priority > best_priority) {
            best_rule = &rule;
            best_priority = rule.priority;
        }
    }

    if (best_rule) {
        cell.terrain_key = best_rule->terrain_key;
        cell.tag_keys = best_rule->tag_keys;
    } else {
        cell.terrain_key = "plain";
    }

    cell.blocks_movement = blocksMovement(cell.terrain_key);
    cell.movement_cost = movementCost(cell.terrain_key);
    cell.biome_kind = biomeKindFromTerrain(cell.terrain_key);

    // Merge derived tags
    auto derived = deriveTags(cell.terrain_key, cell.blocks_movement, sample);
    for (const auto& tag : derived) {
        // Avoid duplicates
        bool exists = false;
        for (const auto& existing : cell.tag_keys) {
            if (existing == tag) { exists = true; break; }
        }
        if (!exists) {
            cell.tag_keys.push_back(tag);
        }
    }

    return cell;
}

} // namespace pathfinder::world_generation
