#pragma once

#include "pathfinder/world_generation/world_generation_types.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainClassifier - converts noise sample into terrain draft
// ---------------------------------------------------------------------------

class TerrainClassifier {
public:
    static GeneratedCellDraft classify(
        const WorldGenerationRequest& request,
        const WorldgenProfile& profile,
        const TerrainNoiseSample& sample,
        const std::string& region_id);

    static bool blocksMovement(const std::string& terrain_key);
    static int movementCost(const std::string& terrain_key);
    static WorldBiomeKind biomeKindFromTerrain(const std::string& terrain_key);

private:
    static std::vector<std::string> deriveTags(
        const std::string& terrain_key,
        bool blocks_movement,
        const TerrainNoiseSample& sample);
};

} // namespace pathfinder::world_generation
