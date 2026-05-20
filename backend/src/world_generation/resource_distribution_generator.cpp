#include "pathfinder/world_generation/resource_distribution_generator.h"
#include <cmath>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool ResourceDistributionGenerator::isTerrainAllowed(
    const GeneratedCellDraft& cell,
    const std::vector<std::string>& allowed_terrain_tags) {
    if (allowed_terrain_tags.empty()) return true;
    // Check if terrain_key is in allowed list
    for (const auto& tag : allowed_terrain_tags) {
        if (cell.terrain_key == tag) return true;
        // Also check tag_keys on cell
        for (const auto& cell_tag : cell.tag_keys) {
            if (cell_tag == tag) return true;
        }
    }
    return false;
}

int ResourceDistributionGenerator::manhattanDistance(const world_runtime::WorldCellCoord& a, const world_runtime::WorldCellCoord& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

// ---------------------------------------------------------------------------
// Main generation
// ---------------------------------------------------------------------------

ResourceDistributionGenerator::Result ResourceDistributionGenerator::generate(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    const std::vector<GeneratedCellDraft>& cells,
    WorldGenerationRng& rng) {
    Result result;

    world_runtime::WorldCellCoord spawn_coord{0, 0, profile.default_layer};

    for (const auto& rule : profile.resource_rules) {
        // Collect candidate cells
        std::vector<const GeneratedCellDraft*> candidates;
        for (const auto& cell : cells) {
            if (!isTerrainAllowed(cell, rule.allowed_terrain_tags)) continue;
            int dist = manhattanDistance(cell.coord, spawn_coord);
            if (dist < rule.min_distance_from_spawn) continue;
            if (rule.max_distance_from_spawn >= 0 && dist > rule.max_distance_from_spawn) continue;
            candidates.push_back(&cell);
        }

        if (candidates.empty()) continue;

        // Determine how many to place based on density
        int max_placements = static_cast<int>(std::ceil(candidates.size() * rule.density));
        if (max_placements <= 0) continue;

        int placed = 0;
        // Shuffle candidates using rng by doing Fisher-Yates style selection
        std::vector<bool> used(candidates.size(), false);
        for (size_t i = 0; i < candidates.size() && placed < max_placements; ++i) {
            // Pick a random unused candidate
            int remaining = static_cast<int>(candidates.size() - i);
            int pick = rng.drawInt(0, remaining - 1);
            size_t idx = 0;
            int count = 0;
            for (size_t j = 0; j < candidates.size(); ++j) {
                if (used[j]) continue;
                if (count == pick) {
                    idx = j;
                    break;
                }
                ++count;
            }
            if (used[idx]) {
                // Fallback: find first unused
                for (size_t j = 0; j < candidates.size(); ++j) {
                    if (!used[j]) { idx = j; break; }
                }
            }
            used[idx] = true;

            const auto& cell = *candidates[idx];

            // Create resource node draft
            GeneratedResourceNodeDraft node;
            node.node_id = makeStableResourceNodeId(rule.resource_key, cell.coord, placed);
            node.resource_key = rule.resource_key;
            node.coord = cell.coord;
            node.node_kind = rule.node_kind;
            node.state = ResourceNodeState::Available;
            node.remaining_charges = rule.charges;
            node.max_charges = rule.charges;
            node.required_action_key = rule.required_action_key;
            node.required_tool_key = rule.required_tool_key;
            node.output_entity_keys = rule.output_entity_keys;
            node.tag_keys = rule.tags;
            result.resource_nodes.push_back(std::move(node));

            result.trace_roll_keys.push_back("resource_" + rule.resource_key + "_placed_" + std::to_string(placed));
            ++placed;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Ground item generation (P46)
// ---------------------------------------------------------------------------

std::vector<GeneratedEntityDraft> ResourceDistributionGenerator::generateGroundItems(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    const std::vector<GeneratedCellDraft>& cells,
    WorldGenerationRng& rng) {
    std::vector<GeneratedEntityDraft> result;

    world_runtime::WorldCellCoord spawn_coord{0, 0, profile.default_layer};

    for (const auto& rule : profile.ground_item_rules) {
        std::vector<const GeneratedCellDraft*> candidates;
        for (const auto& cell : cells) {
            if (!isTerrainAllowed(cell, rule.allowed_terrain_tags)) continue;
            int dist = manhattanDistance(cell.coord, spawn_coord);
            if (dist < rule.min_distance_from_spawn) continue;
            if (rule.max_distance_from_spawn >= 0 && dist > rule.max_distance_from_spawn) continue;
            candidates.push_back(&cell);
        }

        if (candidates.empty()) continue;

        int max_placements = static_cast<int>(std::ceil(candidates.size() * rule.density));
        if (max_placements <= 0) continue;

        int placed = 0;
        std::vector<bool> used(candidates.size(), false);
        for (size_t i = 0; i < candidates.size() && placed < max_placements; ++i) {
            int remaining = static_cast<int>(candidates.size() - i);
            int pick = rng.drawInt(0, remaining - 1);
            size_t idx = 0;
            int count = 0;
            for (size_t j = 0; j < candidates.size(); ++j) {
                if (used[j]) continue;
                if (count == pick) {
                    idx = j;
                    break;
                }
                ++count;
            }
            if (used[idx]) {
                for (size_t j = 0; j < candidates.size(); ++j) {
                    if (!used[j]) { idx = j; break; }
                }
            }
            used[idx] = true;

            const auto& cell = *candidates[idx];

            GeneratedEntityDraft item;
            item.entity_id = makeStableEntityId(rule.entity_key, cell.coord, placed);
            item.entity_key = rule.entity_key;
            item.display_name_key = rule.display_name_key;
            item.coord = cell.coord;
            item.quantity = rule.quantity;
            item.stackable = rule.stackable;
            item.stack_key = rule.stack_key.empty() ? rule.entity_key + ":default" : rule.stack_key;
            item.state_keys = rule.state_keys;
            item.numeric_states = rule.numeric_states;
            item.tag_keys = rule.tags;
            result.push_back(std::move(item));

            ++placed;
        }
    }

    return result;
}

} // namespace pathfinder::world_generation
