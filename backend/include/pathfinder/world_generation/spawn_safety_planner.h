#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/world_generation_rng.h"
#include <vector>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// SpawnSafetyPlanner - ensures spawn area has minimum playable resources
// ---------------------------------------------------------------------------

class SpawnSafetyPlanner {
public:
    struct Result {
        std::vector<GeneratedResourceNodeDraft> resource_nodes;
        std::vector<GeneratedEntityDraft> ground_items;
        std::vector<std::string> trace_roll_keys;
        bool ok = true;
    };

    static Result ensure(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile,
        const std::vector<GeneratedCellDraft>& cells,
        std::vector<GeneratedResourceNodeDraft> existing_nodes,
        std::vector<GeneratedEntityDraft> existing_ground_items,
        WorldGenerationRng& rng);

private:
    static bool hasResourceInRadius(
        const world_runtime::WorldCellCoord& center,
        int radius,
        const std::vector<std::string>& tags,
        const std::vector<GeneratedResourceNodeDraft>& nodes,
        const std::vector<GeneratedEntityDraft>& ground_items);

    static world_runtime::WorldCellCoord findSafeCell(
        const std::vector<GeneratedCellDraft>& cells,
        const world_runtime::WorldCellCoord& center,
        int radius,
        WorldGenerationRng& rng);
};

} // namespace pathfinder::world_generation
