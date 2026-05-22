#include "pathfinder/world_generation/terrain_connectivity_analyzer.h"
#include <queue>
#include <map>
#include <cmath>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainConnectivityAnalyzer
// ---------------------------------------------------------------------------

TerrainGenerationDiagnostics TerrainConnectivityAnalyzer::analyze(
    const std::vector<GeneratedCellDraft>& cells,
    const world_runtime::WorldCellCoord& spawn_coord,
    const TerrainConnectivityPolicy& policy) {

    TerrainGenerationDiagnostics diag;
    diag.total_cells = static_cast<int>(cells.size());

    // Build coord -> index map
    std::map<std::string, int> coord_index;
    for (size_t i = 0; i < cells.size(); ++i) {
        coord_index[cells[i].coord.cellId()] = static_cast<int>(i);
    }

    // Count walkable/blocked
    for (const auto& cell : cells) {
        if (cell.blocks_movement) {
            ++diag.blocked_cells;
        } else {
            ++diag.walkable_cells;
        }
    }

    if (diag.total_cells > 0) {
        diag.walkable_ratio = static_cast<double>(diag.walkable_cells) / static_cast<double>(diag.total_cells);
    }

    // Find spawn cell
    auto spawn_it = coord_index.find(spawn_coord.cellId());
    if (spawn_it == coord_index.end()) {
        diag.spawn_cell_walkable = false;
        diag.warning_keys.push_back("worldgen_spawn_cell_missing");
        return diag;
    }

    const auto& spawn_cell = cells[spawn_it->second];
    diag.spawn_cell_walkable = !spawn_cell.blocks_movement;

    // Count blocked in spawn radius
    int spawn_blocked = 0;
    int spawn_total = 0;
    for (const auto& cell : cells) {
        int dist = std::abs(cell.coord.x - spawn_coord.x) + std::abs(cell.coord.y - spawn_coord.y);
        if (dist <= policy.spawn_safe_radius) {
            ++spawn_total;
            if (cell.blocks_movement) {
                ++spawn_blocked;
            }
        }
    }

    if (spawn_total > 0) {
        diag.blocked_ratio_in_spawn_radius = static_cast<double>(spawn_blocked) / static_cast<double>(spawn_total);
    }

    // BFS from spawn
    std::vector<bool> visited(cells.size(), false);
    std::queue<int> q;

    if (!spawn_cell.blocks_movement) {
        q.push(spawn_it->second);
        visited[spawn_it->second] = true;
    }

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    while (!q.empty()) {
        int idx = q.front();
        q.pop();
        ++diag.reachable_from_spawn;

        const auto& cell = cells[idx];
        for (int dir = 0; dir < 4; ++dir) {
            world_runtime::WorldCellCoord neighbor_coord{
                cell.coord.x + dx[dir],
                cell.coord.y + dy[dir],
                cell.coord.layer_key
            };
            auto neighbor_it = coord_index.find(neighbor_coord.cellId());
            if (neighbor_it == coord_index.end()) continue;
            int nidx = neighbor_it->second;
            if (visited[nidx]) continue;
            if (cells[nidx].blocks_movement) continue;
            visited[nidx] = true;
            q.push(nidx);
        }
    }

    return diag;
}

} // namespace pathfinder::world_generation
