#include "pathfinder/world_generation/terrain_noise_field_builder.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// TerrainNoiseFieldBuilder
// ---------------------------------------------------------------------------

TerrainNoiseField TerrainNoiseFieldBuilder::build(
    const WorldGenerationRequest& request,
    const WorldgenProfile& profile) {

    TerrainNoiseField field;
    TerrainNoiseSampler sampler(request.world_seed);

    int radius = request.region_size / 2;
    int min_c = -(request.region_size / 2);
    int max_c = request.region_size / 2 - (request.region_size % 2 == 0 ? 1 : 0);
    if (request.region_size % 2 == 1) {
        min_c = -radius;
        max_c = radius;
    }

    const std::string& layer_key = profile.default_layer;

    for (int cx = min_c; cx <= max_c; ++cx) {
        for (int cy = min_c; cy <= max_c; ++cy) {
            int world_x = request.region_coord.rx * request.region_size + cx;
            int world_y = request.region_coord.ry * request.region_size + cy;

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
