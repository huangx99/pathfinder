#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/world_generation_rng.h"
#include <vector>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainGenerator - generates cell drafts from profile and seed
// ---------------------------------------------------------------------------

class TerrainGenerator {
public:
    struct Result {
        std::vector<GeneratedCellDraft> cells;
        std::vector<std::string> trace_roll_keys;
        bool ok = true;
    };

    // Generate terrain for a region
    static Result generate(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile,
        WorldGenerationRng& rng);

private:
    static std::string pickTerrain(
        WorldGenerationRng& rng,
        const std::vector<TerrainWeight>& weights,
        int& out_roll);

    static bool blocksMovement(const std::string& terrain_key);
    static int movementCost(const std::string& terrain_key);
    static WorldBiomeKind biomeKindFromTerrain(const std::string& terrain_key);
};

} // namespace pathfinder::world_generation
