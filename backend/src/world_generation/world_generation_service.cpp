#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/terrain_generator.h"
#include "pathfinder/world_generation/resource_distribution_generator.h"
#include "pathfinder/world_generation/spawn_safety_planner.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// Built-in profiles
// ---------------------------------------------------------------------------

static WorldgenProfile buildFirstWorldProfile() {
    WorldgenProfile profile;
    profile.profile_key = "first_world";
    profile.region_size = 16;
    profile.default_layer = "surface";

    profile.terrain_weights = {
        {"plain", 50},
        {"forest", 30},
        {"stone_field", 15},
        {"water_edge", 5}
    };

    profile.resource_rules = {
        {
            "berry_bush",
            {"plain", "forest_edge"},
            0.08,
            1, 6,
            {"food_basic"},
            ResourceNodeKind::Plant,
            "gather", "",
            {"red_berry"},
            3
        },
        {
            "young_tree",
            {"forest", "plain"},
            0.06,
            1, 8,
            {"wood_basic"},
            ResourceNodeKind::Tree,
            "chop", "",
            {"wood"},
            2
        },
        {
            "loose_stone_node",
            {"stone_field", "plain"},
            0.05,
            1, 8,
            {"stone_basic"},
            ResourceNodeKind::Stone,
            "gather", "",
            {"stone_flake"},
            2
        }
    };

    profile.ground_item_rules = {
        {
            "strange_tracks",
            "entity.strange_tracks",
            {"plain", "forest"},
            0.02,
            0, -1,
            1,
            false,
            "strange_tracks:default",
            {"hint"},
            {},
            {}
        }
    };

    profile.spawn_safety.safe_radius = 2;
    profile.spawn_safety.basic_food_min_count = 1;
    profile.spawn_safety.basic_material_min_count = 2;
    profile.spawn_safety.tool_hint_min_count = 1;
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
    GeneratedSpawnPointDraft player_spawn;
    player_spawn.spawn_id = "spawn_player_0_0";
    player_spawn.spawn_kind = SpawnPointKind::PlayerStart;
    player_spawn.coord = world_runtime::WorldCellCoord{0, 0, profile->default_layer};
    player_spawn.actor_key = "player";
    result.spawn_point_drafts.push_back(std::move(player_spawn));

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
