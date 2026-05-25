#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace pathfinder::world_runtime {

// ---------------------------------------------------------------------------
// 9. Core Enums
// ---------------------------------------------------------------------------

enum class WorldLayerPolicy {
    SurfaceOnly,        // 当前世界只启用 surface
    ExplicitLayerOnly,  // 只能在已生成 layer 内移动
    AllowLayerSpawn     // 未来可动态生成 layer
};

std::string toString(WorldLayerPolicy policy);

enum class WorldCellVisibility {
    Unknown,      // 从未见过
    Discovered,   // 见过但当前不可见
    Visible       // 当前可见
};

std::string toString(WorldCellVisibility visibility);

enum class WorldEntityLocationKind {
    Nowhere,      // 未放置或已移除
    OnMap,        // 在地图格子上
    InInventory,  // 在背包中，P45 接管
    InContainer,  // 在容器或营地库存中，P45/P后续接管
    Equipped      // 装备在 Actor 身上，P45/P后续接管
};

std::string toString(WorldEntityLocationKind kind);

enum class WorldMoveBlockReason {
    None,
    OutOfBounds,
    UnknownLayer,
    NotAdjacent,
    CellBlocked,
    EntityBlocked,
    ActorMissing,
    TargetMissing
};

std::string toString(WorldMoveBlockReason reason);

// ---------------------------------------------------------------------------
// 10. Core DTOs / Data Structures
// ---------------------------------------------------------------------------

struct WorldRuntimeConfig {
    std::string world_id;
    uint64_t seed = 0;
    int initial_region_radius = 1;
    int region_size = 16;
    int default_vision_radius = 3;
    std::string generator_version = "1.0.0";
    std::string content_version = "1.0.0";
    std::string worldgen_profile_key = "sandbox_blank";
    bool create_player_entity = true;
    bool player_is_controlled = true;
    WorldLayerPolicy layer_policy = WorldLayerPolicy::SurfaceOnly;
};

struct WorldCellCoord {
    int x = 0;
    int y = 0;
    std::string layer_key = "surface";

    std::string cellId() const;
    bool operator==(const WorldCellCoord& other) const;
    bool operator<(const WorldCellCoord& other) const;
};

struct WorldCellRuntime {
    std::string cell_id;
    WorldCellCoord coord;
    std::string terrain_key;
    std::string region_id;
    bool generated = false;
    bool blocks_movement = false;
    int movement_cost = 1;
    std::vector<std::string> entity_ids;
    std::vector<std::string> tag_keys;
};

struct WorldEntityInstance {
    std::string entity_id;
    std::string entity_key;
    std::string display_name_key;
    WorldEntityLocationKind location_kind = WorldEntityLocationKind::Nowhere;
    std::optional<WorldCellCoord> coord;
    std::string owner_ref;
    std::string container_ref;
    bool blocks_movement = false;
    bool visible_by_default = true;
    std::vector<std::string> tag_keys;
    // P45: stack and state support
    int quantity = 1;
    bool stackable = true;
    std::string stack_key;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};

struct WorldActorRuntime {
    std::string actor_key;
    std::string entity_id;
    WorldCellCoord coord;
    int vision_radius = 3;
    bool is_player_controlled = false;
    int max_health = 10;
    int health = 10;
    bool alive = true;
};

struct ActorHealthChangeResult {
    bool ok = false;
    std::string actor_key;
    std::string entity_id;
    int previous_health = 0;
    int new_health = 0;
    int max_health = 0;
    bool alive = false;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> reason_keys;
};

struct WorldExplorationState {
    std::string actor_key;
    std::map<std::string, WorldCellVisibility> cell_visibility_by_id;
};

struct WorldResourceNodeRuntime {
    std::string node_id;
    std::string resource_key;
    WorldCellCoord coord;
    std::string node_kind_str;
    std::string node_state_str;
    int remaining_charges = 1;
    int max_charges = 1;
    std::string required_action_key;
    std::string required_tool_key;
    std::vector<std::string> output_entity_keys;
    std::vector<std::string> tag_keys;
};

struct WorldRuntimeSnapshot {
    std::string world_id;
    uint64_t seed = 0;
    uint64_t world_tick = 0;
    uint64_t state_version = 0;
    std::map<std::string, WorldCellRuntime> cells;
    std::map<std::string, WorldEntityInstance> entities;
    std::map<std::string, WorldActorRuntime> actors;
    std::map<std::string, WorldResourceNodeRuntime> resource_nodes;
};

// ---------------------------------------------------------------------------
// 11. Result structures for IWorldRuntime methods
// ---------------------------------------------------------------------------

struct MoveActorResult {
    bool moved = false;
    WorldCellCoord from;
    WorldCellCoord to;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    WorldMoveBlockReason block_reason = WorldMoveBlockReason::None;
};

struct InspectWorldResult {
    bool found = false;
    std::string inspected_cell_id;
    std::string inspected_entity_id;
    std::string description_text;
};

struct AdvanceWorldTimeResult {
    uint64_t previous_tick = 0;
    uint64_t new_tick = 0;
    uint64_t new_state_version = 0;
};

} // namespace pathfinder::world_runtime
