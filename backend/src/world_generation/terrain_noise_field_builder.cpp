#include "pathfinder/world_generation/terrain_noise_field_builder.h"
#include "pathfinder/world_generation/world_region_math.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainNoiseFieldBuilder
// ---------------------------------------------------------------------------

TerrainNoiseField TerrainNoiseFieldBuilder::build(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile) {

    TerrainNoiseField field;
    TerrainNoiseSampler sampler(request.world_seed);

    int min_c = WorldRegionMath::regionMinLocalCoord(request.region_size);
    int max_c = WorldRegionMath::regionMaxLocalCoord(request.region_size);

    const std::string& layer_key = profile.default_layer;

    for (int cx = min_c; cx <= max_c; ++cx) {
        for (int cy = min_c; cy <= max_c; ++cy) {
            auto coord = WorldRegionMath::regionCoordToWorld(request.region_coord, cx, cy, request.region_size, layer_key);
            int world_x = coord.x;
            int world_y = coord.y;

            TerrainNoiseSample sample;
            sample.x = world_x;
            sample.y = world_y;
            sample.layer_key = layer_key;

            for (const auto& channel_config : profile.noise_channels) {
                double value = sampler.sample(world_x, world_y, layer_key, channel_config);
                switch (channel_config.channel) {
                    case NoiseChannelKind::Elevation:
                        sample.elevation = value;
                        break;
                    case NoiseChannelKind::Moisture:
                        sample.moisture = value;
                        break;
                    case NoiseChannelKind::Temperature:
                        // P57: reserved
                        break;
                    case NoiseChannelKind::Roughness:
                        sample.roughness = value;
                        break;
                    case NoiseChannelKind::ResourceRichness:
                        sample.resource_richness = value;
                        break;
                    case NoiseChannelKind::DangerPressure:
                        sample.danger_pressure = value;
                        break;
                    default:
                        break;
                }
            }

            field.samples.push_back(std::move(sample));
        }
    }

    return field;
}

} // namespace pathfinder::world_generation
