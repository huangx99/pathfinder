#pragma once

#include "pathfinder/world_runtime/world_runtime_types.h"
#include <cstdint>
#include <random>
#include <string>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Deterministic RNG for world generation
// ---------------------------------------------------------------------------

class WorldGenerationRng {
public:
    // Initialize from world_seed + region_coord + generator_version hash
    explicit WorldGenerationRng(uint64_t world_seed, int region_x, int region_y, const std::string& generator_version);

    // Draw a random integer in [min, max]
    int drawInt(int min, int max);

    // Draw a random double in [0.0, 1.0)
    double drawDouble();

    // Current draw index for traceability
    uint64_t drawIndex() const { return draw_index_; }

private:
    uint64_t draw_index_ = 0;
    std::mt19937_64 engine_;
};

// ---------------------------------------------------------------------------
// Stable ID generators
// ---------------------------------------------------------------------------

std::string makeStableCellId(const world_runtime::WorldCellCoord& coord);
std::string makeStableEntityId(const std::string& entity_key, const world_runtime::WorldCellCoord& coord, int local_index = 0);
std::string makeStableResourceNodeId(const std::string& resource_key, const world_runtime::WorldCellCoord& coord, int local_index = 0);
uint64_t makeRegionSeed(uint64_t world_seed, int region_x, int region_y, const std::string& generator_version);

} // namespace pathfinder::world_generation
