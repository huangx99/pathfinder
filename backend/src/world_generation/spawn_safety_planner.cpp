#include "pathfinder/world_generation/spawn_safety_planner.h"
#include <cmath>
#include <algorithm>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool SpawnSafetyPlanner::hasResourceInRadius(
    const world_runtime::WorldCellCoord& center,
    int radius,
    const std::vector<std::string>& tags,
    const std::vector<GeneratedResourceNodeDraft>& nodes,
    const std::vector<GeneratedEntityDraft>& ground_items) {
    for (const auto& node : nodes) {
        int dist = std::abs(node.coord.x - center.x) + std::abs(node.coord.y - center.y);
        if (dist <= radius) {
            for (const auto& tag : tags) {
                for (const auto& node_tag : node.tag_keys) {
                    if (node_tag == tag) return true;
                }
                if (node.resource_key == tag) return true;
            }
        }
    }
    for (const auto& item : ground_items) {
        int dist = std::abs(item.coord.x - center.x) + std::abs(item.coord.y - center.y);
        if (dist <= radius) {
            for (const auto& tag : tags) {
                if (item.entity_key == tag) return true;
                for (const auto& item_tag : item.tag_keys) {
                    if (item_tag == tag) return true;
                }
            }
        }
    }
    return false;
}

world_runtime::WorldCellCoord SpawnSafetyPlanner::findSafeCell(
    const std::vector<GeneratedCellDraft>& cells,
    const world_runtime::WorldCellCoord& center,
    int radius,
    WorldGenerationRng& rng) {
    std::vector<world_runtime::WorldCellCoord> candidates;
    for (const auto& cell : cells) {
        if (cell.blocks_movement) continue;
        int dist = std::abs(cell.coord.x - center.x) + std::abs(cell.coord.y - center.y);
        if (dist <= radius) {
            candidates.push_back(cell.coord);
        }
    }
    if (candidates.empty()) {
        // Fallback: pick any non-blocked cell in the region
        for (const auto& cell : cells) {
            if (!cell.blocks_movement) {
                candidates.push_back(cell.coord);
            }
        }
        if (candidates.empty()) {
            return center;
        }
    }
    int pick = rng.drawInt(0, static_cast<int>(candidates.size()) - 1);
    return candidates[pick];
}

// ---------------------------------------------------------------------------
// Main ensure
// ---------------------------------------------------------------------------

SpawnSafetyPlanner::Result SpawnSafetyPlanner::ensure(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile,
    const std::vector<GeneratedCellDraft>& cells,
    std::vector<GeneratedResourceNodeDraft> existing_nodes,
    std::vector<GeneratedEntityDraft> existing_ground_items,
    WorldGenerationRng& rng) {
    Result result;
    result.resource_nodes = std::move(existing_nodes);
    result.ground_items = std::move(existing_ground_items);

    world_runtime::WorldCellCoord spawn_coord{0, 0, profile.default_layer};
    const auto& policy = profile.spawn_safety;

    // Helper: find first resource rule matching any of the given tags
    auto findRuleByTag = [&](const std::vector<std::string>& target_tags) -> const ResourceDistributionRule* {
        for (const auto& rule : profile.resource_rules) {
            for (const auto& tag : rule.tags) {
                for (const auto& target : target_tags) {
                    if (tag == target) return &rule;
                }
            }
        }
        return nullptr;
    };

    // Ensure basic food
    if (policy.basic_food_min_count > 0) {
        int food_count = 0;
        for (const auto& node : result.resource_nodes) {
            int dist = std::abs(node.coord.x - spawn_coord.x) + std::abs(node.coord.y - spawn_coord.y);
            if (dist <= policy.safe_radius) {
                for (const auto& tag : node.tag_keys) {
                    if (tag == "food_basic") { ++food_count; break; }
                }
            }
        }
        for (const auto& item : result.ground_items) {
            int dist = std::abs(item.coord.x - spawn_coord.x) + std::abs(item.coord.y - spawn_coord.y);
            if (dist <= policy.safe_radius) {
                for (const auto& tag : item.tag_keys) {
                    if (tag == "food_basic") { ++food_count; break; }
                }
            }
        }
        if (food_count < policy.basic_food_min_count) {
            auto coord = findSafeCell(cells, spawn_coord, policy.safe_radius, rng);
            const auto* rule = findRuleByTag({"food_basic"});
            if (rule) {
                GeneratedResourceNodeDraft node;
                node.node_id = makeStableResourceNodeId(rule->resource_key, coord, 99);
                node.resource_key = rule->resource_key;
                node.coord = coord;
                node.node_kind = rule->node_kind;
                node.state = ResourceNodeState::Available;
                node.remaining_charges = rule->charges;
                node.max_charges = rule->charges;
                node.required_action_key = rule->required_action_key;
                node.required_tool_key = rule->required_tool_key;
                node.output_entity_keys = rule->output_entity_keys;
                node.tag_keys = rule->tags;
                result.resource_nodes.push_back(std::move(node));
                result.trace_roll_keys.push_back("safety_fallback_food_" + coord.cellId());
            } else {
                result.trace_roll_keys.push_back("safety_fallback_food_missing_rule");
            }
        }
    }

    // Ensure basic materials (wood + stone)
    if (policy.basic_material_min_count > 0) {
        bool has_wood = hasResourceInRadius(spawn_coord, policy.safe_radius, {"wood_basic"}, result.resource_nodes, result.ground_items);
        bool has_stone = hasResourceInRadius(spawn_coord, policy.safe_radius, {"stone_basic"}, result.resource_nodes, result.ground_items);

        if (!has_wood) {
            auto coord = findSafeCell(cells, spawn_coord, policy.safe_radius, rng);
            const auto* rule = findRuleByTag({"wood_basic"});
            if (rule) {
                GeneratedResourceNodeDraft node;
                node.node_id = makeStableResourceNodeId(rule->resource_key, coord, 99);
                node.resource_key = rule->resource_key;
                node.coord = coord;
                node.node_kind = rule->node_kind;
                node.state = ResourceNodeState::Available;
                node.remaining_charges = rule->charges;
                node.max_charges = rule->charges;
                node.required_action_key = rule->required_action_key;
                node.required_tool_key = rule->required_tool_key;
                node.output_entity_keys = rule->output_entity_keys;
                node.tag_keys = rule->tags;
                result.resource_nodes.push_back(std::move(node));
                result.trace_roll_keys.push_back("safety_fallback_wood_" + coord.cellId());
            } else {
                result.trace_roll_keys.push_back("safety_fallback_wood_missing_rule");
            }
        }

        if (!has_stone) {
            auto coord = findSafeCell(cells, spawn_coord, policy.safe_radius, rng);
            const auto* rule = findRuleByTag({"stone_basic"});
            if (rule) {
                GeneratedResourceNodeDraft node;
                node.node_id = makeStableResourceNodeId(rule->resource_key, coord, 99);
                node.resource_key = rule->resource_key;
                node.coord = coord;
                node.node_kind = rule->node_kind;
                node.state = ResourceNodeState::Available;
                node.remaining_charges = rule->charges;
                node.max_charges = rule->charges;
                node.required_action_key = rule->required_action_key;
                node.required_tool_key = rule->required_tool_key;
                node.output_entity_keys = rule->output_entity_keys;
                node.tag_keys = rule->tags;
                result.resource_nodes.push_back(std::move(node));
                result.trace_roll_keys.push_back("safety_fallback_stone_" + coord.cellId());
            } else {
                result.trace_roll_keys.push_back("safety_fallback_stone_missing_rule");
            }
        }
    }

    // Ensure tool hint: only from profile rules (resource_rules or ground_item_rules) with "hint" tag
    if (policy.tool_hint_min_count > 0) {
        bool has_hint = hasResourceInRadius(spawn_coord, policy.safe_radius, {"hint"}, result.resource_nodes, result.ground_items);
        if (!has_hint) {
            auto coord = findSafeCell(cells, spawn_coord, policy.safe_radius, rng);
            const auto* rule = findRuleByTag({"hint"});
            if (rule) {
                GeneratedResourceNodeDraft node;
                node.node_id = makeStableResourceNodeId(rule->resource_key, coord, 99);
                node.resource_key = rule->resource_key;
                node.coord = coord;
                node.node_kind = rule->node_kind;
                node.state = ResourceNodeState::Available;
                node.remaining_charges = rule->charges;
                node.max_charges = rule->charges;
                node.required_action_key = rule->required_action_key;
                node.required_tool_key = rule->required_tool_key;
                node.output_entity_keys = rule->output_entity_keys;
                node.tag_keys = rule->tags;
                result.resource_nodes.push_back(std::move(node));
                result.trace_roll_keys.push_back("safety_fallback_hint_" + coord.cellId());
            } else {
                // Fallback to ground_item_rules with hint tag
                const GroundItemPlacementRule* gi_rule = nullptr;
                for (const auto& gir : profile.ground_item_rules) {
                    for (const auto& tag : gir.tags) {
                        if (tag == "hint") { gi_rule = &gir; break; }
                    }
                    if (gi_rule) break;
                }
                if (gi_rule) {
                    GeneratedEntityDraft item;
                    item.entity_id = makeStableEntityId(gi_rule->entity_key, coord, 99);
                    item.entity_key = gi_rule->entity_key;
                    item.display_name_key = gi_rule->display_name_key;
                    item.coord = coord;
                    item.quantity = gi_rule->quantity;
                    item.stackable = gi_rule->stackable;
                    item.stack_key = gi_rule->stack_key.empty() ? gi_rule->entity_key + ":default" : gi_rule->stack_key;
                    item.state_keys = gi_rule->state_keys;
                    item.numeric_states = gi_rule->numeric_states;
                    item.tag_keys = gi_rule->tags;
                    result.ground_items.push_back(std::move(item));
                    result.trace_roll_keys.push_back("safety_fallback_hint_" + coord.cellId());
                } else {
                    result.trace_roll_keys.push_back("safety_fallback_hint_missing_rule");
                }
            }
        }
    }

    return result;
}

} // namespace pathfinder::world_generation
