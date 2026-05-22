#include "pathfinder/world_generation/terrain_connectivity_repairer.h"
#include "pathfinder/world_generation/terrain_classifier.h"
#include "pathfinder/world_generation/terrain_connectivity_analyzer.h"
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static double distSq(int x1, int y1, int x2, int y2) {
    double dx = static_cast<double>(x1 - x2);
    double dy = static_cast<double>(y1 - y2);
    return dx * dx + dy * dy;
}

static std::string pickRepairTerrain(const std::vector<std::string>& preferred) {
    if (!preferred.empty()) return preferred[0];
    return "plain";
}

// ---------------------------------------------------------------------------
// TerrainConnectivityRepairer
// ---------------------------------------------------------------------------

TerrainGenerationDiagnostics TerrainConnectivityRepairer::repair(
    std::vector<GeneratedCellDraft>& cells,
    const world_runtime::WorldCellCoord& spawn_coord,
    const TerrainConnectivityPolicy& policy) {

    TerrainGenerationDiagnostics diag;
    if (!policy.enabled) {
        diag = TerrainConnectivityAnalyzer::analyze(cells, spawn_coord, policy);
        return diag;
    }

    // Build coord -> index
    std::map<std::string, int> coord_index;
    for (size_t i = 0; i < cells.size(); ++i) {
        coord_index[cells[i].coord.cellId()] = static_cast<int>(i);
    }

    auto spawn_it = coord_index.find(spawn_coord.cellId());
    if (spawn_it == coord_index.end()) {
        diag.warning_keys.push_back("worldgen_spawn_cell_missing");
        return diag;
    }

    int repaired = 0;
    std::string repair_terrain = pickRepairTerrain(policy.repair_preferred_terrain_keys);

    // Step 1: Force spawn cell walkable
    {
        auto& spawn_cell = cells[spawn_it->second];
        if (spawn_cell.blocks_movement) {
            spawn_cell.terrain_key = repair_terrain;
            spawn_cell.blocks_movement = false;
            spawn_cell.movement_cost = TerrainClassifier::movementCost(repair_terrain);
            spawn_cell.biome_kind = TerrainClassifier::biomeKindFromTerrain(repair_terrain);
            // Update tags: replace blocked with walkable
            for (auto& tag : spawn_cell.tag_keys) {
                if (tag == "blocked") tag = "walkable";
                if (tag.find("terrain:") == 0) tag = "terrain:" + repair_terrain;
            }
            bool has_walkable = false;
            for (const auto& tag : spawn_cell.tag_keys) {
                if (tag == "walkable") has_walkable = true;
            }
            if (!has_walkable) spawn_cell.tag_keys.push_back("walkable");
            ++repaired;
        }
    }

    // Step 2: Force spawn_safe_radius cells walkable
    for (auto& cell : cells) {
        int dist = std::abs(cell.coord.x - spawn_coord.x) + std::abs(cell.coord.y - spawn_coord.y);
        if (dist <= policy.spawn_safe_radius && cell.blocks_movement) {
            cell.terrain_key = repair_terrain;
            cell.blocks_movement = false;
            cell.movement_cost = TerrainClassifier::movementCost(repair_terrain);
            cell.biome_kind = TerrainClassifier::biomeKindFromTerrain(repair_terrain);
            for (auto& tag : cell.tag_keys) {
                if (tag == "blocked") tag = "walkable";
                if (tag.find("terrain:") == 0) tag = "terrain:" + repair_terrain;
            }
            bool has_walkable = false;
            for (const auto& tag : cell.tag_keys) {
                if (tag == "walkable") has_walkable = true;
            }
            if (!has_walkable) cell.tag_keys.push_back("walkable");
            ++repaired;
        }
    }

    // Step 3: Carve cardinal corridors if enabled
    if (policy.carve_cardinal_corridors) {
        const int directions[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        for (int dir = 0; dir < 4; ++dir) {
            for (int step = 1; ; ++step) {
                int tx = spawn_coord.x + directions[dir][0] * step;
                int ty = spawn_coord.y + directions[dir][1] * step;
                world_runtime::WorldCellCoord target{tx, ty, spawn_coord.layer_key};
                auto it = coord_index.find(target.cellId());
                if (it == coord_index.end()) break; // out of region
                auto& cell = cells[it->second];
                if (cell.blocks_movement) {
                    cell.terrain_key = repair_terrain;
                    cell.blocks_movement = false;
                    cell.movement_cost = TerrainClassifier::movementCost(repair_terrain);
                    cell.biome_kind = TerrainClassifier::biomeKindFromTerrain(repair_terrain);
                    for (auto& tag : cell.tag_keys) {
                        if (tag == "blocked") tag = "walkable";
                        if (tag.find("terrain:") == 0) tag = "terrain:" + repair_terrain;
                    }
                    bool has_walkable = false;
                    for (const auto& tag : cell.tag_keys) {
                        if (tag == "walkable") has_walkable = true;
                    }
                    if (!has_walkable) cell.tag_keys.push_back("walkable");
                    ++repaired;
                }
            }
        }
    }

    // Re-analyze after initial fixes
    diag = TerrainConnectivityAnalyzer::analyze(cells, spawn_coord, policy);

    // Step 5: If reachable still insufficient, carve to nearest unreachable walkable cluster
    if (diag.reachable_from_spawn < policy.min_reachable_cells_from_spawn) {
        // BFS from spawn to find reachable set
        std::vector<bool> reachable(cells.size(), false);
        std::queue<int> q;
        q.push(spawn_it->second);
        reachable[spawn_it->second] = true;

        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};

        while (!q.empty()) {
            int idx = q.front();
            q.pop();
            const auto& cell = cells[idx];
            for (int dir = 0; dir < 4; ++dir) {
                world_runtime::WorldCellCoord neighbor_coord{
                    cell.coord.x + dx[dir],
                    cell.coord.y + dy[dir],
                    cell.coord.layer_key
                };
                auto it = coord_index.find(neighbor_coord.cellId());
                if (it == coord_index.end()) continue;
                int nidx = it->second;
                if (reachable[nidx]) continue;
                if (cells[nidx].blocks_movement) continue;
                reachable[nidx] = true;
                q.push(nidx);
            }
        }

        // Collect unreachable walkable cells
        std::vector<int> unreachable_walkable;
        for (size_t i = 0; i < cells.size(); ++i) {
            if (!reachable[i] && !cells[i].blocks_movement) {
                unreachable_walkable.push_back(static_cast<int>(i));
            }
        }

        // For each unreachable cluster (simplified: single-cell greedy), carve path
        // Sort by distance to spawn for deterministic order
        std::sort(unreachable_walkable.begin(), unreachable_walkable.end(),
            [&](int a, int b) {
                double da = distSq(cells[a].coord.x, cells[a].coord.y, spawn_coord.x, spawn_coord.y);
                double db = distSq(cells[b].coord.x, cells[b].coord.y, spawn_coord.x, spawn_coord.y);
                if (da != db) return da < db;
                if (cells[a].coord.x != cells[b].coord.x) return cells[a].coord.x < cells[b].coord.x;
                return cells[a].coord.y < cells[b].coord.y;
            });

        for (int target_idx : unreachable_walkable) {
            if (diag.reachable_from_spawn >= policy.min_reachable_cells_from_spawn) break;

            // Trace a line from spawn to target, unblock cells along the way
            int tx = cells[target_idx].coord.x;
            int ty = cells[target_idx].coord.y;
            int sx = spawn_coord.x;
            int sy = spawn_coord.y;

            // Simple axis-first path: move x then y
            int cx = sx;
            int cy = sy;
            bool changed = false;

            while (cx != tx || cy != ty) {
                if (cx != tx) {
                    cx += (tx > cx) ? 1 : -1;
                } else if (cy != ty) {
                    cy += (ty > cy) ? 1 : -1;
                }

                world_runtime::WorldCellCoord step_coord{cx, cy, spawn_coord.layer_key};
                auto it = coord_index.find(step_coord.cellId());
                if (it == coord_index.end()) continue;
                auto& cell = cells[it->second];
                if (cell.blocks_movement) {
                    cell.terrain_key = repair_terrain;
                    cell.blocks_movement = false;
                    cell.movement_cost = TerrainClassifier::movementCost(repair_terrain);
                    cell.biome_kind = TerrainClassifier::biomeKindFromTerrain(repair_terrain);
                    for (auto& tag : cell.tag_keys) {
                        if (tag == "blocked") tag = "walkable";
                        if (tag.find("terrain:") == 0) tag = "terrain:" + repair_terrain;
                    }
                    bool has_walkable = false;
                    for (const auto& tag : cell.tag_keys) {
                        if (tag == "walkable") has_walkable = true;
                    }
                    if (!has_walkable) cell.tag_keys.push_back("walkable");
                    ++repaired;
                    changed = true;
                }
            }

            if (changed) {
                // Re-run BFS to update reachable set
                std::fill(reachable.begin(), reachable.end(), false);
                while (!q.empty()) q.pop();
                q.push(spawn_it->second);
                reachable[spawn_it->second] = true;
                while (!q.empty()) {
                    int idx = q.front();
                    q.pop();
                    const auto& cell = cells[idx];
                    for (int dir = 0; dir < 4; ++dir) {
                        world_runtime::WorldCellCoord neighbor_coord{
                            cell.coord.x + dx[dir],
                            cell.coord.y + dy[dir],
                            cell.coord.layer_key
                        };
                        auto it = coord_index.find(neighbor_coord.cellId());
                        if (it == coord_index.end()) continue;
                        int nidx = it->second;
                        if (reachable[nidx]) continue;
                        if (cells[nidx].blocks_movement) continue;
                        reachable[nidx] = true;
                        q.push(nidx);
                    }
                }
                // Update reachable count
                int count = 0;
                for (bool r : reachable) if (r) ++count;
                diag.reachable_from_spawn = count;
            }
        }
    }

    // Step 6: If walkable ratio still too low, unblock cells from spawn outward
    diag = TerrainConnectivityAnalyzer::analyze(cells, spawn_coord, policy);
    if (diag.walkable_ratio < policy.min_walkable_ratio) {
        // Sort blocked cells by distance to spawn
        std::vector<int> blocked_indices;
        for (size_t i = 0; i < cells.size(); ++i) {
            if (cells[i].blocks_movement) {
                blocked_indices.push_back(static_cast<int>(i));
            }
        }
        std::sort(blocked_indices.begin(), blocked_indices.end(),
            [&](int a, int b) {
                double da = distSq(cells[a].coord.x, cells[a].coord.y, spawn_coord.x, spawn_coord.y);
                double db = distSq(cells[b].coord.x, cells[b].coord.y, spawn_coord.x, spawn_coord.y);
                if (da != db) return da < db;
                if (cells[a].coord.x != cells[b].coord.x) return cells[a].coord.x < cells[b].coord.x;
                return cells[a].coord.y < cells[b].coord.y;
            });

        for (int idx : blocked_indices) {
            if (diag.walkable_ratio >= policy.min_walkable_ratio) break;
            auto& cell = cells[idx];
            cell.terrain_key = repair_terrain;
            cell.blocks_movement = false;
            cell.movement_cost = TerrainClassifier::movementCost(repair_terrain);
            cell.biome_kind = TerrainClassifier::biomeKindFromTerrain(repair_terrain);
            for (auto& tag : cell.tag_keys) {
                if (tag == "blocked") tag = "walkable";
                if (tag.find("terrain:") == 0) tag = "terrain:" + repair_terrain;
            }
            bool has_walkable = false;
            for (const auto& tag : cell.tag_keys) {
                if (tag == "walkable") has_walkable = true;
            }
            if (!has_walkable) cell.tag_keys.push_back("walkable");
            ++repaired;
            ++diag.walkable_cells;
            --diag.blocked_cells;
            diag.walkable_ratio = static_cast<double>(diag.walkable_cells) / static_cast<double>(diag.total_cells);
        }
    }

    diag = TerrainConnectivityAnalyzer::analyze(cells, spawn_coord, policy);
    diag.connectivity_repaired = repaired > 0;
    diag.repaired_cells = repaired;

    if (repaired > 0) {
        diag.warning_keys.push_back("worldgen_connectivity_repaired");
    }
    if (!diag.spawn_cell_walkable) {
        diag.warning_keys.push_back("worldgen_spawn_cell_was_blocked");
    }
    if (diag.walkable_ratio < policy.min_walkable_ratio) {
        diag.warning_keys.push_back("worldgen_walkable_ratio_low_after_repair");
    }
    if (diag.reachable_from_spawn < policy.min_reachable_cells_from_spawn) {
        diag.warning_keys.push_back("worldgen_reachable_cells_low_after_repair");
    }

    return diag;
}

} // namespace pathfinder::world_generation
