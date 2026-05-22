#include "pathfinder/world_generation/terrain_noise_sampler.h"
#include <cmath>
#include <algorithm>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// SplitMix64 to expand a seed into a good pseudo-random sequence
static uint64_t splitMix64(uint64_t& state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// Deterministic float in [0,1) from uint64
static double u01(uint64_t v) {
    // Use upper 53 bits for double precision
    return static_cast<double>(v >> 11) / static_cast<double>(1ULL << 53);
}

// Linear interpolation
static double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

// Smoothstep for better interpolation
static double smoothstep(double t) {
    return t * t * (3.0 - 2.0 * t);
}

// ---------------------------------------------------------------------------
// TerrainNoiseSampler
// ---------------------------------------------------------------------------

TerrainNoiseSampler::TerrainNoiseSampler(uint64_t world_seed)
    : world_seed_(world_seed) {
}

uint64_t TerrainNoiseSampler::hashCoord(int x, int y, uint64_t salt) {
    // Deterministic 2D coordinate hash with good avalanche properties
    uint64_t hash = salt;
    hash += static_cast<uint64_t>(x) * 0x9e3779b97f4a7c15ULL;
    hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
    hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
    hash ^= hash >> 31;
    hash += static_cast<uint64_t>(y) * 0x9e3779b97f4a7c15ULL;
    hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
    hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
    hash ^= hash >> 31;
    return hash;
}

double TerrainNoiseSampler::valueNoise2D(double x, double y, uint64_t salt) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    double fx = x - static_cast<double>(ix);
    double fy = y - static_cast<double>(iy);

    double u = smoothstep(fx);
    double v = smoothstep(fy);

    uint64_t s00 = hashCoord(ix,     iy,     salt);
    uint64_t s10 = hashCoord(ix + 1, iy,     salt);
    uint64_t s01 = hashCoord(ix,     iy + 1, salt);
    uint64_t s11 = hashCoord(ix + 1, iy + 1, salt);

    double n00 = u01(s00);
    double n10 = u01(s10);
    double n01 = u01(s01);
    double n11 = u01(s11);

    double nx0 = lerp(n00, n10, u);
    double nx1 = lerp(n01, n11, u);
    double val = lerp(nx0, nx1, v);

    // Normalize from [0,1) to [-1,1)
    return val * 2.0 - 1.0;
}

double TerrainNoiseSampler::fractalValueNoise2D(double x, double y, uint64_t salt,
                                                   int octaves, double persistence, double lacunarity) {
    double total = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double max_value = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += amplitude * valueNoise2D(x * frequency, y * frequency, salt + static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    if (max_value > 0.0) {
        total /= max_value;
    }
    return total;
}

// ---------------------------------------------------------------------------
// Perlin (gradient) noise
// ---------------------------------------------------------------------------

// Deterministic 2D gradient from hash value: unit vector at hash-determined angle
static void hashGradient2D(uint64_t hash, double& gx, double& gy) {
    // Use hash to pick an angle, then cos/sin for a unit gradient
    double angle = static_cast<double>(hash & 0xFFFFFFFFULL) / static_cast<double>(0x100000000ULL) * 6.283185307179586;
    gx = std::cos(angle);
    gy = std::sin(angle);
}

double TerrainNoiseSampler::perlin2D(double x, double y, uint64_t salt) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    double fx = x - static_cast<double>(ix);
    double fy = y - static_cast<double>(iy);

    double u = smoothstep(fx);
    double v = smoothstep(fy);

    // Gradients at the four lattice corners
    double gx00, gy00, gx10, gy10, gx01, gy01, gx11, gy11;
    hashGradient2D(hashCoord(ix,     iy,     salt), gx00, gy00);
    hashGradient2D(hashCoord(ix + 1, iy,     salt), gx10, gy10);
    hashGradient2D(hashCoord(ix,     iy + 1, salt), gx01, gy01);
    hashGradient2D(hashCoord(ix + 1, iy + 1, salt), gx11, gy11);

    // Distance vectors from corners to sample point
    double n00 = gx00 * fx     + gy00 * fy;
    double n10 = gx10 * (fx - 1.0) + gy10 * fy;
    double n01 = gx01 * fx     + gy01 * (fy - 1.0);
    double n11 = gx11 * (fx - 1.0) + gy11 * (fy - 1.0);

    // Bilinear interpolation of the dot products
    double nx0 = lerp(n00, n10, u);
    double nx1 = lerp(n01, n11, u);
    double val = lerp(nx0, nx1, v);

    // Perlin noise output is roughly [-sqrt(2)/2, sqrt(2)/2].
    // Scale to [-1, 1] for consistency with value noise.
    return val * 1.4142135623730951;
}

double TerrainNoiseSampler::fractalPerlin2D(double x, double y, uint64_t salt,
                                             int octaves, double persistence, double lacunarity) {
    double total = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double max_value = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += amplitude * perlin2D(x * frequency, y * frequency, salt + static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
        max_value += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    if (max_value > 0.0) {
        total /= max_value;
    }
    return total;
}

uint64_t TerrainNoiseSampler::makeSalt(const std::string& layer_key, uint64_t channel_salt) const {
    uint64_t hash = world_seed_;
    for (char c : layer_key) {
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }
    hash ^= channel_salt + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
    return hash;
}

double TerrainNoiseSampler::sample(
    int world_x,
    int world_y,
    const std::string& layer_key,
    const NoiseChannelConfig& config) const {

    uint64_t salt = makeSalt(layer_key, config.salt);
    double nx = static_cast<double>(world_x) / config.scale;
    double ny = static_cast<double>(world_y) / config.scale;

    double value = 0.0;

    switch (config.algorithm) {
        case NoiseAlgorithmKind::ValueNoise2D:
            value = valueNoise2D(nx, ny, salt);
            break;
        case NoiseAlgorithmKind::Perlin2D:
            value = perlin2D(nx, ny, salt);
            break;
        case NoiseAlgorithmKind::FractalPerlin2D:
            value = fractalPerlin2D(nx, ny, salt,
                                     config.octaves,
                                     config.persistence,
                                     config.lacunarity);
            break;
        default:
            value = 0.0;
            break;
    }

    value = value * config.weight + config.bias;
    // Clamp to [-1, 1] to avoid overflow
    return std::max(-1.0, std::min(1.0, value));
}

} // namespace pathfinder::world_generation
