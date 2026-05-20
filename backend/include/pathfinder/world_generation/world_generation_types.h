#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// 8. Core Enums
// ---------------------------------------------------------------------------

enum class WorldGenerationFailureKind {
    None,
    InvalidRequest,
    ContentMissing,
    ProfileNotFound,
    RegionAlreadyGenerated,
    InvalidRegionCoord,
    InvalidLayer,
    DeterminismMismatch,
    ApplyFailed,
    RuntimeConflict
};

std::string toString(WorldGenerationFailureKind kind);

enum class WorldBiomeKind {
    Unknown,
    Plains,
    Forest,
    Wetland,
    StoneField,
    Cave,
    Ruins,
    WaterEdge,
    DangerNest
};

std::string toString(WorldBiomeKind kind);

enum class ResourceNodeKind {
    Unknown,
    Plant,
    Tree,
    Stone,
    Ore,
    Soil,
    Water,
    Corpse,
    Nest,
    Relic
};

std::string toString(ResourceNodeKind kind);

enum class ResourceNodeState {
    Unknown,
    Available,
    Hidden,
    Depleted,
    Regenerating,
    Blocked,
    Dangerous
};

std::string toString(ResourceNodeState kind);

enum class SpawnPointKind {
    Unknown,
    PlayerStart,
    CompanionStart,
    NpcCamp,
    BeastNest,
    ResourceCluster,
    DangerEntrance,
    Landmark
};

std::string toString(SpawnPointKind kind);

// ---------------------------------------------------------------------------
// 9. Core DTOs
// ---------------------------------------------------------------------------

struct WorldRegionCoord {
    int rx = 0;
    int ry = 0;

    bool operator==(const WorldRegionCoord& other) const {
        return rx == other.rx && ry == other.ry;
    }
    bool operator<(const WorldRegionCoord& other) const {
        if (rx != other.rx) return rx < other.rx;
        return ry < other.ry;
    }
    std::string regionId() const {
        return "region_" + std::to_string(rx) + "_" + std::to_string(ry);
    }
};

struct WorldGenerationRequest {
    std::string world_id;
    uint64_t world_seed = 0;
    std::string generator_version;
    std::string content_version;
    std::string worldgen_profile_key;
    WorldRegionCoord region_coord;
    int region_size = 16;
    std::vector<std::string> enabled_layer_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldGenerationManifest {
    std::string world_id;
    uint64_t world_seed = 0;
    std::string generator_version;
    std::string content_version;
    std::string worldgen_profile_key;
    WorldRegionCoord region_coord;
    int region_size = 16;
    std::vector<std::string> enabled_layer_keys;
    std::vector<std::string> generated_cell_ids;
    std::vector<std::string> generated_entity_ids;
    std::vector<std::string> generated_resource_node_ids;
    std::vector<std::string> trace_roll_keys;
    uint64_t generation_timestamp = 0;
};

struct GeneratedCellDraft {
    world_runtime::WorldCellCoord coord;
    std::string terrain_key;
    std::string region_id;
    bool blocks_movement = false;
    int movement_cost = 1;
    std::vector<std::string> tag_keys;
    WorldBiomeKind biome_kind = WorldBiomeKind::Unknown;
};

struct GeneratedEntityDraft {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    world_runtime::WorldCellCoord coord;
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
    std::vector<std::string> tag_keys;
};

struct GeneratedResourceNodeDraft {
    std::string node_id;
    std::string resource_key;
    world_runtime::WorldCellCoord coord;
    ResourceNodeKind node_kind = ResourceNodeKind::Unknown;
    ResourceNodeState state = ResourceNodeState::Available;
    int remaining_charges = 1;
    int max_charges = 1;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_entity_keys;
    std::vector<std::string> tag_keys;
};

struct GeneratedSpawnPointDraft {
    std::string spawn_id;
    SpawnPointKind spawn_kind = SpawnPointKind::Unknown;
    world_runtime::WorldCellCoord coord;
    std::string actor_key;
    std::vector<std::string> tag_keys;
};

struct GeneratedDangerZoneDraft {
    std::string zone_id;
    std::string danger_key;
    world_runtime::WorldCellCoord coord;
    int radius = 0;
    std::vector<std::string> tag_keys;
};

struct WorldGenerationResult {
    bool ok = false;
    WorldGenerationFailureKind failure_kind = WorldGenerationFailureKind::None;
    WorldGenerationManifest manifest;
    std::vector<GeneratedCellDraft> cell_drafts;
    std::vector<GeneratedEntityDraft> entity_drafts;
    std::vector<GeneratedResourceNodeDraft> resource_node_drafts;
    std::vector<GeneratedSpawnPointDraft> spawn_point_drafts;
    std::vector<GeneratedDangerZoneDraft> danger_zone_drafts;
    std::vector<world_command::WorldEventDto> events;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct SpawnSafetyPolicy {
    int safe_radius = 2;
    int basic_food_min_count = 1;
    int basic_material_min_count = 2;
    int tool_hint_min_count = 1;
    int immediate_threat_max_count = 0;
    std::vector<std::string> guaranteed_resource_keys;
    std::vector<std::string> forbidden_danger_keys;
};

struct TerrainWeight {
    std::string terrain_key;
    int weight = 0;
};

struct ResourceDistributionRule {
    std::string resource_key;
    std::vector<std::string> allowed_terrain_tags;
    double density = 0.0;
    int min_distance_from_spawn = 0;
    int max_distance_from_spawn = -1; // -1 means unlimited
    std::vector<std::string> tags;
    ResourceNodeKind node_kind = ResourceNodeKind::Unknown;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_entity_keys;
    int charges = 1;
};

struct GroundItemPlacementRule {
    std::string entity_key;
    std::string display_name_key;
    std::vector<std::string> allowed_terrain_tags;
    double density = 0.0;
    int min_distance_from_spawn = 0;
    int max_distance_from_spawn = -1; // -1 means unlimited
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> tags;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};

struct WorldgenProfile {
    std::string profile_key;
    int region_size = 16;
    std::string default_layer = "surface";
    std::vector<TerrainWeight> terrain_weights;
    std::vector<ResourceDistributionRule> resource_rules;
    std::vector<GroundItemPlacementRule> ground_item_rules;
    SpawnSafetyPolicy spawn_safety;
};

// ---------------------------------------------------------------------------
// 10. Resource Node Runtime (P46 draft, consumed by P47)
// ---------------------------------------------------------------------------

struct ResourceNodeRuntime {
    std::string node_id;
    std::string resource_key;
    world_runtime::WorldCellCoord coord;
    ResourceNodeKind node_kind = ResourceNodeKind::Unknown;
    ResourceNodeState state = ResourceNodeState::Available;
    int remaining_charges = 1;
    int max_charges = 1;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_entity_keys;
    std::vector<std::string> tag_keys;
};

// ---------------------------------------------------------------------------
// 11. Applier result
// ---------------------------------------------------------------------------

struct WorldGenerationApplyResult {
    bool ok = false;
    WorldGenerationFailureKind failure_kind = WorldGenerationFailureKind::None;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<world_command::WorldEventDto> events;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
};

} // namespace pathfinder::world_generation
