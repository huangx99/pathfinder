#include "pathfinder/world_generation/world_generation_rng.h"
#include <functional>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Hash combine helper
// ---------------------------------------------------------------------------

static uint64_t hashCombine(uint64_t seed, uint64_t value) {
    // Based on boost::hash_combine
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

static uint64_t hashString(const std::string& str) {
    uint64_t hash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
    for (char c : str) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// ---------------------------------------------------------------------------
// WorldGenerationRng
// ---------------------------------------------------------------------------

WorldGenerationRng::WorldGenerationRng(uint64_t world_seed, int region_x, int region_y, const std::string& generator_version)
    : draw_index_(0) {
    uint64_t seed = makeRegionSeed(world_seed, region_x, region_y, generator_version);
    engine_.seed(seed);
}

int WorldGenerationRng::drawInt(int min, int max) {
    if (min > max) std::swap(min, max);
    std::uniform_int_distribution<int> dist(min, max);
    ++draw_index_;
    return dist(engine_);
}

double WorldGenerationRng::drawDouble() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    ++draw_index_;
    return dist(engine_);
}

// ---------------------------------------------------------------------------
// Stable ID generators
// ---------------------------------------------------------------------------

std::string makeStableCellId(const world_runtime::WorldCellCoord& coord) {
    return coord.layer_key + ":" + std::to_string(coord.x) + ":" + std::to_string(coord.y);
}

std::string makeStableEntityId(const std::string& entity_key, const world_runtime::WorldCellCoord& coord, int local_index) {
    return entity_key + "_" + coord.layer_key + "_" + std::to_string(coord.x) + "_" + std::to_string(coord.y)
           + (local_index > 0 ? "_" + std::to_string(local_index) : "");
}

std::string makeStableResourceNodeId(const std::string& resource_key, const world_runtime::WorldCellCoord& coord, int local_index) {
    return "node_" + resource_key + "_" + coord.layer_key + "_" + std::to_string(coord.x) + "_" + std::to_string(coord.y)
           + (local_index > 0 ? "_" + std::to_string(local_index) : "");
}

uint64_t makeRegionSeed(uint64_t world_seed, int region_x, int region_y, const std::string& generator_version) {
    uint64_t seed = world_seed;
    seed = hashCombine(seed, static_cast<uint64_t>(region_x));
    seed = hashCombine(seed, static_cast<uint64_t>(region_y));
    seed = hashCombine(seed, hashString(generator_version));
    return seed;
}

} // namespace pathfinder::world_generation
