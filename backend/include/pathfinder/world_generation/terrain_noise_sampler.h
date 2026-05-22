#pragma once

#include "pathfinder/world_generation/world_generation_types.h"
#include <cstdint>
#include <string>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainNoiseSampler - deterministic noise for world coordinates
// ---------------------------------------------------------------------------

class TerrainNoiseSampler {
public:
    explicit TerrainNoiseSampler(uint64_t world_seed);

    // Sample a noise channel at world coordinates.
    // Output is normalized to approximately [-1, 1].
    double sample(
        int world_x,
        int world_y,
        const std::string& layer_key,
        const NoiseChannelConfig& config) const;

private:
    uint64_t world_seed_;

    // Deterministic hash for a coordinate + salt combination
    static uint64_t hashCoord(int x, int y, uint64_t salt);

    // 2D value noise at integer lattice points, bilinearly interpolated
    static double valueNoise2D(double x, double y, uint64_t salt);

    // Fractal value noise (octaves)
    static double fractalValueNoise2D(double x, double y, uint64_t salt,
                                       int octaves, double persistence, double lacunarity);

    // 2D gradient (Perlin) noise at integer lattice points
    static double perlin2D(double x, double y, uint64_t salt);

    // Fractal gradient noise (octaves)
    static double fractalPerlin2D(double x, double y, uint64_t salt,
                                   int octaves, double persistence, double lacunarity);

    // Combine world_seed, layer_key, and channel salt into final salt
    uint64_t makeSalt(const std::string& layer_key, uint64_t channel_salt) const;
};

} // namespace pathfinder::world_generation
