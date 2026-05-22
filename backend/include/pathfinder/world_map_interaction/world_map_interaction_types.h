#pragma once

#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/world_generation/world_region_ensure_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <map>

namespace pathfinder::world_map_interaction {

// ---------------------------------------------------------------------------
// Selection / Action enums
// ---------------------------------------------------------------------------

enum class MapSelectionKind {
    Unknown,
    Cell,
    Entity,
    ResourceNode,
    GroundItem,
    InventoryItem,
    TestOnly
};

std::string toString(MapSelectionKind kind);
pathfinder::foundation::Result<MapSelectionKind> mapSelectionKindFromString(const std::string& str);

enum class MapActionOptionStatus {
    Unknown,
    Available,
    Disabled,
    Hidden,
    TestOnly
};

std::string toString(MapActionOptionStatus status);
pathfinder::foundation::Result<MapActionOptionStatus> mapActionOptionStatusFromString(const std::string& str);

// ---------------------------------------------------------------------------
// 8.1 RegionActivityWindow
// ---------------------------------------------------------------------------

struct RegionActivityWindow {
    std::string world_id;
    std::string layer_key;
    std::string center_actor_key;
    world_runtime::WorldCellCoord center_coord;
    int viewport_min_x = 0;
    int viewport_max_x = 0;
    int viewport_min_y = 0;
    int viewport_max_y = 0;
    int vision_radius = 3;
    int preload_margin_regions = 1;
    std::vector<world_generation::WorldRegionKey> keep_active_regions;
    std::vector<world_generation::WorldRegionKey> prewarm_regions;
    std::vector<world_generation::WorldRegionKey> seal_candidate_regions;
    std::vector<world_generation::WorldRegionKey> detach_candidate_regions;
    std::vector<world_generation::WorldRegionKey> blocked_regions;
    std::vector<std::string> reason_keys;
    uint64_t computed_tick = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientMapInteractionContext
// ---------------------------------------------------------------------------

struct ClientMapInteractionContext {
    std::string session_id;
    std::string actor_key;
    std::string layer_key;
    world_runtime::WorldCellCoord center_coord;
    int viewport_min_x = 0;
    int viewport_max_x = 0;
    int viewport_min_y = 0;
    int viewport_max_y = 0;
    uint64_t projection_version = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientMapSelectionRequest / Response
// ---------------------------------------------------------------------------

struct ClientMapSelectionRequest {
    std::string session_id;
    std::string client_id;
    uint64_t known_projection_version = 0;
    MapSelectionKind selection_kind = MapSelectionKind::Unknown;
    world_runtime::WorldCellCoord cell_coord;
    std::string entity_id;
    std::string resource_node_id;
    std::string inventory_item_id;
    std::string layer_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientMapSelectedObjectSummary {
    std::string object_id;
    std::string object_kind; // "entity" / "resource_node" / "ground_item"
    std::string display_name_key;
    std::string display_name_zh;
    std::vector<std::string> tag_keys;
    std::vector<std::string> safe_summary_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientMapSelectionResponse {
    std::string session_id;
    std::string selection_id;
    bool valid = false;
    bool stale = false;
    bool requires_full_refresh = false;
    world_runtime::WorldCellCoord selected_cell;
    std::string selected_cell_terrain_key;
    std::vector<ClientMapSelectedObjectSummary> selected_objects;
    std::vector<std::string> allowed_target_kinds;
    std::vector<pathfinder::world_command::WorldCommandOptionDto> available_options;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientMapActionOptionDto
// ---------------------------------------------------------------------------

struct ClientMapActionOptionDto {
    std::string option_id;
    std::string command_key;
    std::string label_zh;
    std::string short_hint_zh;
    std::string target_summary;
    MapActionOptionStatus status = MapActionOptionStatus::Unknown;
    std::string disabled_reason_zh;
    std::optional<std::string> cost_preview;
    std::optional<std::string> risk_preview;
    int priority = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientMapPathPreviewDto
// ---------------------------------------------------------------------------

struct ClientMapPathPreviewDto {
    bool reachable = false;
    int estimated_steps = 0;
    std::vector<world_runtime::WorldCellCoord> path_cells;
    std::string next_move_command_key;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientRegionLifecycleHintDto / EventDto
// ---------------------------------------------------------------------------

struct ClientRegionLifecycleHintDto {
    std::string region_runtime_id;
    std::string hint_kind; // "keep_active" / "seal_candidate" / "detached" / "restored" / "generated"
    std::string label_zh;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ClientRegionLifecycleEventDto {
    std::string event_id;
    uint64_t tick = 0;
    std::string event_kind; // "sealed" / "detached" / "restored" / "generated" / "restore_failed"
    std::string region_runtime_id;
    std::string label_zh;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientKnowledgeFeedbackDto
// ---------------------------------------------------------------------------

struct ClientKnowledgeFeedbackDto {
    std::string feedback_id;
    std::string actor_key;
    std::string knowledge_key;
    std::string subject_entity_key;
    std::string label_zh;
    std::string description_zh;
    std::string confidence_label;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 10.2 ClientMapProjectionDto (extended projection for map interaction)
// ---------------------------------------------------------------------------

struct ClientMapProjectionDto {
    uint64_t projection_version = 0;
    std::string actor_key;
    std::string active_layer_key;
    world_runtime::WorldCellCoord viewport_center;
    int viewport_radius = 0;
    std::vector<pathfinder::world_command::WorldCellPatchDto> visible_cells;
    std::vector<pathfinder::world_command::WorldEntityPatchDto> visible_entities;
    std::vector<pathfinder::world_command::InventoryPatchDto> inventories;
    std::vector<pathfinder::world_command::KnowledgePatchDto> knowledge;
    std::vector<pathfinder::world_command::AreaEffectPatchDto> area_effects;
    std::vector<ClientRegionLifecycleHintDto> region_lifecycle_hints;
    std::vector<std::string> safe_summary_keys;
    std::vector<std::string> warning_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::world_map_interaction
