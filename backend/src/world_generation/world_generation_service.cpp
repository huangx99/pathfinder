#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/terrain_generator.h"
#include "pathfinder/world_generation/resource_distribution_generator.h"
#include "pathfinder/world_generation/spawn_safety_planner.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/content/content_registry.h"
#include <algorithm>

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;


namespace {

TerrainGenerationMode terrainGenerationModeFromContentString(const std::string& value) {
    if (value == "WeightedRandom" || value == "weighted_random") return TerrainGenerationMode::WeightedRandom;
    if (value == "NoiseField" || value == "noise_field") return TerrainGenerationMode::NoiseField;
    if (value == "TestOnly" || value == "test_only") return TerrainGenerationMode::TestOnly;
    return TerrainGenerationMode::Unknown;
}

NoiseChannelKind noiseChannelKindFromContentString(const std::string& value) {
    if (value == "Elevation" || value == "elevation") return NoiseChannelKind::Elevation;
    if (value == "Moisture" || value == "moisture") return NoiseChannelKind::Moisture;
    if (value == "Temperature" || value == "temperature") return NoiseChannelKind::Temperature;
    if (value == "Roughness" || value == "roughness") return NoiseChannelKind::Roughness;
    if (value == "ResourceRichness" || value == "resource_richness") return NoiseChannelKind::ResourceRichness;
    if (value == "DangerPressure" || value == "danger_pressure") return NoiseChannelKind::DangerPressure;
    if (value == "TestOnly" || value == "test_only") return NoiseChannelKind::TestOnly;
    return NoiseChannelKind::Unknown;
}

NoiseAlgorithmKind noiseAlgorithmKindFromContentString(const std::string& value) {
    if (value == "ValueNoise2D" || value == "value_noise_2d") return NoiseAlgorithmKind::ValueNoise2D;
    if (value == "Perlin2D" || value == "perlin_2d") return NoiseAlgorithmKind::Perlin2D;
    if (value == "FractalPerlin2D" || value == "fractal_perlin_2d") return NoiseAlgorithmKind::FractalPerlin2D;
    if (value == "TestOnly" || value == "test_only") return NoiseAlgorithmKind::TestOnly;
    return NoiseAlgorithmKind::Unknown;
}

ResourceNodeKind resourceNodeKindFromContentString(const std::string& value) {
    if (value == "Plant" || value == "plant") return ResourceNodeKind::Plant;
    if (value == "Tree" || value == "tree") return ResourceNodeKind::Tree;
    if (value == "Stone" || value == "stone") return ResourceNodeKind::Stone;
    if (value == "Ore" || value == "ore") return ResourceNodeKind::Ore;
    if (value == "Soil" || value == "soil") return ResourceNodeKind::Soil;
    if (value == "Water" || value == "water") return ResourceNodeKind::Water;
    if (value == "Corpse" || value == "corpse") return ResourceNodeKind::Corpse;
    if (value == "Nest" || value == "nest") return ResourceNodeKind::Nest;
    if (value == "Relic" || value == "relic") return ResourceNodeKind::Relic;
    return ResourceNodeKind::Unknown;
}

std::vector<std::string> mergeTags(
    const std::vector<std::string>& base,
    const std::vector<std::string>& extra) {
    std::vector<std::string> result = base;
    for (const auto& tag : extra) {
        if (std::find(result.begin(), result.end(), tag) == result.end()) {
            result.push_back(tag);
        }
    }
    return result;
}

std::map<std::string, double> mergeNumericStates(
    const std::map<std::string, double>& base,
    const std::map<std::string, double>& overrides) {
    auto result = base;
    for (const auto& [key, value] : overrides) {
        result[key] = value;
    }
    return result;
}

} // namespace

// ---------------------------------------------------------------------------
// Built-in profiles
// ---------------------------------------------------------------------------

static WorldgenProfile buildFirstWorldProfile() {
    WorldgenProfile profile;
    profile.profile_key = "first_world";
    profile.region_size = 16;
    profile.default_layer = "surface";

    // P57: default to noise field generation
    profile.terrain_generation_mode = TerrainGenerationMode::NoiseField;

    // Legacy weights kept for compatibility with WeightedRandom mode
    profile.terrain_weights = {
        {"plain", 50},
        {"forest", 30},
        {"stone_field", 15},
        {"water_edge", 5}
    };

    // P57: noise channels
    // Elevation + Moisture use large scales to form natural biomes.
    // Roughness / ResourceRichness / DangerPressure use smaller scales for local detail.
    profile.noise_channels = {
        {NoiseChannelKind::Elevation,        NoiseAlgorithmKind::FractalPerlin2D, 20.0, 3, 0.5, 2.0, 0.0, 1.0, 101},
        {NoiseChannelKind::Moisture,         NoiseAlgorithmKind::FractalPerlin2D, 16.0, 3, 0.5, 2.0, 0.0, 1.0, 202},
        {NoiseChannelKind::Roughness,        NoiseAlgorithmKind::FractalPerlin2D, 5.0,  2, 0.5, 2.0, 0.0, 1.0, 303},
        {NoiseChannelKind::ResourceRichness, NoiseAlgorithmKind::FractalPerlin2D, 4.0,  2, 0.5, 2.0, 0.0, 1.0, 404},
        {NoiseChannelKind::DangerPressure,   NoiseAlgorithmKind::FractalPerlin2D, 8.0,  2, 0.5, 2.0, 0.0, 1.0, 505}
    };

    // P57: terrain threshold rules (higher priority = evaluated first)
    // Thresholds relaxed so small regions still show multiple terrain types.
    profile.terrain_threshold_rules = {
        // blocked: very high elevation and rough
        {"blocked",     0.75,  1.0,   -1.0,  1.0,   0.40,  1.0,   100, {"blocked", "rough"}},
        // mountain: very high elevation
        {"mountain",    0.65,  0.75,  -1.0,  1.0,   -1.0,  1.0,   90,  {"mountain"}},
        // deep_water: very low elevation
        {"deep_water",  -1.0,  -0.65, -1.0,  1.0,   -1.0,  1.0,   80,  {"deep_water", "water"}},
        // water_edge: low elevation
        {"water_edge",  -0.65, -0.30, -1.0,  1.0,   -1.0,  1.0,   70,  {"water_edge", "wet"}},
        // forest: moderate moisture (relaxed lower bound)
        {"forest",      -0.50, 0.60,  0.10,  1.0,   -1.0,  0.50,  60,  {"forest"}},
        // stone_field: high roughness or high elevation
        {"stone_field", -0.50, 0.70,  -1.0,  1.0,   0.30,  1.0,   50,  {"stone_field", "rough"}},
        // plain: default catch-all (lowest priority)
        {"plain",       -1.0,  1.0,   -1.0,  1.0,   -1.0,  1.0,   0,   {"plain"}}
    };

    // P57: connectivity policy
    profile.connectivity_policy.enabled = true;
    profile.connectivity_policy.spawn_safe_radius = 3;
    profile.connectivity_policy.min_walkable_ratio = 0.75;
    profile.connectivity_policy.max_blocked_ratio_in_spawn_radius = 0.05;
    profile.connectivity_policy.min_reachable_cells_from_spawn = 120;
    profile.connectivity_policy.carve_cardinal_corridors = true;
    profile.connectivity_policy.corridor_half_width = 1;
    profile.connectivity_policy.repair_preferred_terrain_keys = {"plain"};

    profile.resource_rules.clear();
    profile.ground_item_rules.clear();

    profile.spawn_safety.safe_radius = 2;
    profile.spawn_safety.basic_food_min_count = 1;
    profile.spawn_safety.basic_material_min_count = 2;
    profile.spawn_safety.tool_hint_min_count = 0;
    profile.spawn_safety.immediate_threat_max_count = 0;
    profile.spawn_safety.guaranteed_resource_keys = {"food_basic", "wood_basic", "stone_basic"};
    profile.spawn_safety.forbidden_danger_keys = {"active_predator"};

    return profile;
}

// ---------------------------------------------------------------------------
// WorldGenerationService
// ---------------------------------------------------------------------------

WorldGenerationService::WorldGenerationService() {
    registerBuiltinProfiles();
}

void WorldGenerationService::registerBuiltinProfiles() {
    registerProfile(buildFirstWorldProfile());
}

void WorldGenerationService::registerProfile(const WorldgenProfile& profile) {
    profiles_[profile.profile_key] = profile;
}


Result<void> WorldGenerationService::registerContentProfiles(const pathfinder::content::ContentRegistry& registry) {
    for (const auto* content_profile : registry.allWorldgenProfiles()) {
        if (!content_profile) continue;

        WorldgenProfile profile;
        profile.profile_key = content_profile->profile_key;
        profile.region_size = content_profile->region_size;
        profile.default_layer = content_profile->default_layer.empty() ? "surface" : content_profile->default_layer;
        profile.terrain_generation_mode = terrainGenerationModeFromContentString(content_profile->terrain_generation_mode);

        for (const auto& weight : content_profile->terrain_weights) {
            profile.terrain_weights.push_back(TerrainWeight{weight.terrain_key, weight.weight});
        }
        for (const auto& channel : content_profile->noise_channels) {
            profile.noise_channels.push_back(NoiseChannelConfig{
                noiseChannelKindFromContentString(channel.channel),
                noiseAlgorithmKindFromContentString(channel.algorithm),
                channel.scale,
                channel.octaves,
                channel.persistence,
                channel.lacunarity,
                channel.bias,
                channel.weight,
                channel.salt
            });
        }
        for (const auto& rule : content_profile->terrain_threshold_rules) {
            profile.terrain_threshold_rules.push_back(TerrainThresholdRule{
                rule.terrain_key,
                rule.min_elevation,
                rule.max_elevation,
                rule.min_moisture,
                rule.max_moisture,
                rule.min_roughness,
                rule.max_roughness,
                rule.priority,
                rule.tag_keys
            });
        }

        profile.connectivity_policy.enabled = content_profile->connectivity_policy.enabled;
        profile.connectivity_policy.spawn_safe_radius = content_profile->connectivity_policy.spawn_safe_radius;
        profile.connectivity_policy.min_walkable_ratio = content_profile->connectivity_policy.min_walkable_ratio;
        profile.connectivity_policy.max_blocked_ratio_in_spawn_radius = content_profile->connectivity_policy.max_blocked_ratio_in_spawn_radius;
        profile.connectivity_policy.min_reachable_cells_from_spawn = content_profile->connectivity_policy.min_reachable_cells_from_spawn;
        profile.connectivity_policy.carve_cardinal_corridors = content_profile->connectivity_policy.carve_cardinal_corridors;
        profile.connectivity_policy.corridor_half_width = content_profile->connectivity_policy.corridor_half_width;
        profile.connectivity_policy.repair_preferred_terrain_keys = content_profile->connectivity_policy.repair_preferred_terrain_keys;

        for (const auto& rule : content_profile->resource_rules) {
            if (!rule.required_tool_key.empty() && !registry.findObject(rule.required_tool_key)) {
                return Result<void>::fail(makeError(ErrorCode::id_not_found, "worldgen_required_tool_missing", rule.required_tool_key));
            }
            for (const auto& output_key : rule.output_object_keys) {
                if (!registry.findObject(output_key)) {
                    return Result<void>::fail(makeError(ErrorCode::id_not_found, "worldgen_output_object_missing", output_key));
                }
            }
            profile.resource_rules.push_back(ResourceDistributionRule{
                rule.resource_key,
                rule.allowed_terrain_tags,
                rule.density,
                rule.min_distance_from_spawn,
                rule.max_distance_from_spawn,
                rule.tag_keys,
                resourceNodeKindFromContentString(rule.node_kind),
                rule.required_action_key,
                rule.required_tool_key,
                rule.output_object_keys,
                rule.charges
            });
        }

        for (const auto& rule : content_profile->ground_item_rules) {
            const auto* object = registry.findObject(rule.object_key);
            if (!object) {
                return Result<void>::fail(makeError(ErrorCode::id_not_found, "worldgen_ground_object_missing", rule.object_key));
            }
            GroundItemPlacementRule ground_rule;
            ground_rule.entity_key = rule.object_key;
            ground_rule.display_name_key = object->display_key;
            ground_rule.allowed_terrain_tags = rule.allowed_terrain_tags;
            ground_rule.density = rule.density;
            ground_rule.min_distance_from_spawn = rule.min_distance_from_spawn;
            ground_rule.max_distance_from_spawn = rule.max_distance_from_spawn;
            ground_rule.quantity = rule.quantity > 0 ? rule.quantity : std::max(1, object->default_quantity);
            ground_rule.stackable = rule.stackable;
            ground_rule.stack_key = rule.stack_key.empty() ? rule.object_key + ":default" : rule.stack_key;
            ground_rule.tags = mergeTags(object->safe_tags, rule.tag_keys);
            ground_rule.state_keys = rule.state_keys;
            ground_rule.numeric_states = mergeNumericStates(object->default_numeric, rule.numeric_states);
            profile.ground_item_rules.push_back(std::move(ground_rule));
        }

        profile.spawn_safety.safe_radius = content_profile->spawn_safety.safe_radius;
        profile.spawn_safety.basic_food_min_count = content_profile->spawn_safety.basic_food_min_count;
        profile.spawn_safety.basic_material_min_count = content_profile->spawn_safety.basic_material_min_count;
        profile.spawn_safety.tool_hint_min_count = content_profile->spawn_safety.tool_hint_min_count;
        profile.spawn_safety.immediate_threat_max_count = content_profile->spawn_safety.immediate_threat_max_count;
        profile.spawn_safety.guaranteed_resource_keys = content_profile->spawn_safety.guaranteed_resource_keys;
        profile.spawn_safety.forbidden_danger_keys = content_profile->spawn_safety.forbidden_danger_keys;

        registerProfile(profile);
    }

    return Result<void>::ok();
}

const WorldgenProfile* WorldGenerationService::findProfile(const std::string& profile_key) const {
    auto it = profiles_.find(profile_key);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool WorldGenerationService::isRegionGenerated(const std::string& world_id, const WorldRegionCoord& coord) const {
    auto it = generated_regions_.find(world_id);
    if (it == generated_regions_.end()) return false;
    return it->second.find(coord) != it->second.end();
}

void WorldGenerationService::markRegionGenerated(const std::string& world_id, const WorldRegionCoord& coord) {
    generated_regions_[world_id].insert(coord);
}

// ---------------------------------------------------------------------------
// Main generation
// ---------------------------------------------------------------------------

WorldGenerationResult WorldGenerationService::generate(const WorldGenerationRequest& request) {
    WorldGenerationResult result;

    auto validate = request.validateBasic();
    if (validate.is_error()) {
        result.failure_kind = WorldGenerationFailureKind::InvalidRequest;
        return result;
    }

    const auto* profile = findProfile(request.worldgen_profile_key);
    if (!profile) {
        result.failure_kind = WorldGenerationFailureKind::ProfileNotFound;
        return result;
    }

    // P46: only support default_layer, reject multi-layer requests
    if (!request.enabled_layer_keys.empty() &&
        (request.enabled_layer_keys.size() > 1 ||
         request.enabled_layer_keys[0] != profile->default_layer)) {
        result.failure_kind = WorldGenerationFailureKind::InvalidLayer;
        return result;
    }

    // Initialize RNG
    WorldGenerationRng rng(
        request.world_seed,
        request.region_coord.rx,
        request.region_coord.ry,
        request.generator_version);

    // Step 1: Generate terrain
    auto terrain_result = TerrainGenerator::generate(request, *profile, rng);
    result.cell_drafts = std::move(terrain_result.cells);
    result.manifest.trace_roll_keys.insert(
        result.manifest.trace_roll_keys.end(),
        terrain_result.trace_roll_keys.begin(),
        terrain_result.trace_roll_keys.end());
    result.manifest.diagnostics = terrain_result.diagnostics;

    // Step 2: Generate resources
    auto resource_result = ResourceDistributionGenerator::generate(request, *profile, result.cell_drafts, rng);
    result.resource_node_drafts = std::move(resource_result.resource_nodes);
    result.manifest.trace_roll_keys.insert(
        result.manifest.trace_roll_keys.end(),
        resource_result.trace_roll_keys.begin(),
        resource_result.trace_roll_keys.end());

    // Step 2b: Generate ground items from profile rules
    auto ground_items = ResourceDistributionGenerator::generateGroundItems(request, *profile, result.cell_drafts, rng);

    // Step 3: Spawn safety
    auto safety_result = SpawnSafetyPlanner::ensure(
        request, *profile, result.cell_drafts,
        std::move(result.resource_node_drafts),
        std::move(ground_items),
        rng);
    result.resource_node_drafts = std::move(safety_result.resource_nodes);
    result.entity_drafts = std::move(safety_result.ground_items);
    result.manifest.trace_roll_keys.insert(
        result.manifest.trace_roll_keys.end(),
        safety_result.trace_roll_keys.begin(),
        safety_result.trace_roll_keys.end());

    // Step 4: Build spawn points
    // P57: only origin region emits player spawn.
    if (request.region_coord.rx == 0 && request.region_coord.ry == 0) {
        GeneratedSpawnPointDraft player_spawn;
        player_spawn.spawn_id = "spawn_player_0_0";
        player_spawn.spawn_kind = SpawnPointKind::PlayerStart;
        player_spawn.coord = world_runtime::WorldCellCoord{0, 0, profile->default_layer};
        player_spawn.actor_key = "player";
        result.spawn_point_drafts.push_back(std::move(player_spawn));
    }

    // Build manifest
    result.manifest.world_id = request.world_id;
    result.manifest.world_seed = request.world_seed;
    result.manifest.generator_version = request.generator_version;
    result.manifest.content_version = request.content_version;
    result.manifest.worldgen_profile_key = request.worldgen_profile_key;
    result.manifest.region_coord = request.region_coord;
    result.manifest.region_size = request.region_size;
    result.manifest.enabled_layer_keys = request.enabled_layer_keys;
    result.manifest.generation_timestamp = 0; // P46: deterministic, not real time

    for (const auto& cell : result.cell_drafts) {
        result.manifest.generated_cell_ids.push_back(cell.coord.cellId());
    }
    for (const auto& entity : result.entity_drafts) {
        result.manifest.generated_entity_ids.push_back(entity.entity_id);
    }
    for (const auto& node : result.resource_node_drafts) {
        result.manifest.generated_resource_node_ids.push_back(node.node_id);
    }

    // Build events
    world_command::WorldEventDto event;
    event.event_id = "worldgen_" + request.world_id + "_" + request.region_coord.regionId();
    event.event_kind = "WorldGenerated";
    event.title_text = "世界区域已生成";
    event.body_text = "Region " + request.region_coord.regionId() + " generated with profile " + request.worldgen_profile_key;
    event.coord = world_command::WorldCoordinateDto{0, 0, profile->default_layer};
    result.events.push_back(std::move(event));

    result.ok = true;
    return result;
}

} // namespace pathfinder::world_generation
