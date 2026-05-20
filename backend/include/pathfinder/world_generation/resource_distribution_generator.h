#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/world_generation_rng.h"
#include <vector>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// ResourceDistributionGenerator - places resource nodes based on biome
// ---------------------------------------------------------------------------

class ResourceDistributionGenerator {
public:
    struct Result {
        std::vector<GeneratedResourceNodeDraft> resource_nodes;
        std::vector<GeneratedEntityDraft> ground_items;
        std::vector<std::string> trace_roll_keys;
        bool ok = true;
    };

    static Result generate(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile,
        const std::vector<GeneratedCellDraft>& cells,
        WorldGenerationRng& rng);

    // P46: generate ground items from profile rules
    static std::vector<GeneratedEntityDraft> generateGroundItems(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile,
        const std::vector<GeneratedCellDraft>& cells,
        WorldGenerationRng& rng);

private:
    static bool isTerrainAllowed(
        const GeneratedCellDraft& cell,
        const std::vector<std::string>& allowed_terrain_tags);

    static int manhattanDistance(const world_runtime::WorldCellCoord& a, const world_runtime::WorldCellCoord& b);
};

} // namespace pathfinder::world_generation
