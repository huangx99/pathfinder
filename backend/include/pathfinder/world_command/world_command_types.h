#pragma once

#include "pathfinder/foundation/result.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_command {

// ---------------------------------------------------------------------------
// 8. Core Enums
// ---------------------------------------------------------------------------

enum class WorldCommandKind {
    Unknown,
    Noop,
    Wait,
    Inspect,
    Move,
    Gather,
    Chop,
    Mine,
    Dig,
    Pickup,
    Drop,
    Eat,
    Use,
    Craft,
    Teach,
    Attack,
    Flee,
    GenerateWorld,
    GenerateRegion,
    SpawnEntity,
    DespawnEntity,
    TriggerAreaEvent,
    ApplyAreaEffectTick,
    CastAreaEffect,
    SystemTick
};

std::string toString(WorldCommandKind kind);
pathfinder::foundation::Result<WorldCommandKind> worldCommandKindFromString(const std::string& str);

enum class WorldCommandSource {
    Unknown,
    PlayerInput,
    AgentDecision,
    BeastDecision,
    SystemTick,
    WorldGeneration,
    AreaEffectTick,
    ReactionChain,
    ScriptedScenario,
    Replay,
    Test
};

std::string toString(WorldCommandSource source);
pathfinder::foundation::Result<WorldCommandSource> worldCommandSourceFromString(const std::string& str);

enum class WorldCommandResultKind {
    Unknown,
    Succeeded,
    Failed,
    Blocked,
    Interrupted,
    Deferred,
    Noop
};

std::string toString(WorldCommandResultKind kind);
pathfinder::foundation::Result<WorldCommandResultKind> worldCommandResultKindFromString(const std::string& str);

enum class WorldCommandTargetKind {
    None,
    Coordinate,
    Entity,
    Actor,
    Item,
    Inventory,
    Area,
    Knowledge,
    Recipe
};

std::string toString(WorldCommandTargetKind kind);
pathfinder::foundation::Result<WorldCommandTargetKind> worldCommandTargetKindFromString(const std::string& str);

enum class PatchOp {
    Add,
    Update,
    Remove,
    Replace,
    Clear
};

std::string toString(PatchOp op);
pathfinder::foundation::Result<PatchOp> patchOpFromString(const std::string& str);

enum class FrontendHintKind {
    HighlightCell,
    HighlightEntity,
    FloatingText,
    ShakeCell,
    PlaySound,
    SpawnEffect,
    RemoveEffect,
    OpenPanel,
    UpdateCommandBar,
    FocusCamera
};

std::string toString(FrontendHintKind kind);
pathfinder::foundation::Result<FrontendHintKind> frontendHintKindFromString(const std::string& str);

enum class AreaShapeKind {
    SingleCell,
    CircleRadius,
    Rectangle,
    Line,
    Cone,
    CustomMask
};

std::string toString(AreaShapeKind kind);
pathfinder::foundation::Result<AreaShapeKind> areaShapeKindFromString(const std::string& str);

// ---------------------------------------------------------------------------
// 9. Core DTOs
// ---------------------------------------------------------------------------

struct WorldCoordinateDto {
    int x = 0;
    int y = 0;
    std::string layer_key = "surface";

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandTargetDto {
    WorldCommandTargetKind target_kind = WorldCommandTargetKind::None;
    std::optional<WorldCoordinateDto> target_coord;
    std::string target_entity_id;
    std::string target_actor_key;
    std::string target_item_key;
    std::string target_inventory_id;
    std::string target_area_id;
    std::string knowledge_key;
    std::string recipe_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandContextDto {
    uint64_t issued_tick = 0;
    uint64_t client_sequence = 0;
    std::string correlation_id;
    std::string parent_command_id;
    std::string actor_location_layer;
    bool allow_hidden_debug = false;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandDto {
    std::string command_id;
    WorldCommandKind command_kind = WorldCommandKind::Unknown;
    std::string command_key;
    WorldCommandSource source = WorldCommandSource::Unknown;
    std::string actor_key;
    WorldCommandTargetDto target;
    WorldCommandContextDto context;
    std::map<std::string, std::string> parameters;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldStateDeltaDto {
    std::string delta_id;
    std::string delta_kind;
    std::string target_ref;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldEventDto {
    std::string event_id;
    std::string event_kind;
    uint64_t tick = 0;
    std::string title_text;
    std::string body_text;
    std::optional<WorldCoordinateDto> coord;
    std::string actor_key;
    std::string target_entity_id;
    int priority = 0;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldExperienceDto {
    std::string experience_id;
    std::string actor_key;
    std::string command_key;
    std::string subject_entity_key;
    std::string target_entity_key;
    std::string effect_key;
    uint64_t tick = 0;
    std::vector<std::string> condition_keys;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandResultDto {
    std::string command_id;
    std::string command_key;
    WorldCommandResultKind result_kind = WorldCommandResultKind::Unknown;
    std::string actor_key;
    std::string target_ref;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;
    std::vector<WorldStateDeltaDto> state_deltas;
    std::vector<WorldCommandDto> spawned_commands;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// Projection Patch DTOs
// ---------------------------------------------------------------------------

struct WorldCellPatchDto {
    std::string cell_id;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldEntityPatchDto {
    std::string entity_id;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct InventoryPatchDto {
    std::string inventory_id;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct KnowledgePatchDto {
    std::string actor_key;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct AreaEffectPatchDto {
    std::string area_effect_id;
    PatchOp op = PatchOp::Update;
    std::map<std::string, std::string> fields;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldProjectionPatchDto {
    uint64_t base_projection_version = 0;
    uint64_t new_projection_version = 0;
    bool requires_full_refresh = false;
    std::vector<WorldCellPatchDto> changed_cells;
    std::vector<WorldEntityPatchDto> changed_entities;
    std::vector<InventoryPatchDto> changed_inventories;
    std::vector<KnowledgePatchDto> changed_knowledge;
    std::vector<AreaEffectPatchDto> changed_area_effects;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldFrontendHintDto {
    FrontendHintKind hint_kind = FrontendHintKind::FloatingText;
    std::string target_entity_id;
    std::optional<WorldCoordinateDto> target_coord;
    std::string text;
    std::string effect_key;
    int priority = 0;
    int duration_ms = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandOptionDto {
    std::string option_id;
    WorldCommandKind command_kind = WorldCommandKind::Unknown;
    std::string command_key;
    std::string label_text;
    WorldCommandTargetDto target;
    bool enabled = true;
    std::string disabled_reason_text;
    int priority = 0;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct WorldCommandResponseDto {
    std::string session_id;
    std::string command_id;
    WorldCommandResultDto result;
    WorldProjectionPatchDto projection_patch;
    std::vector<WorldEventDto> event_feed;
    std::vector<WorldExperienceDto> experiences;
    std::vector<WorldFrontendHintDto> frontend_hints;
    std::vector<WorldCommandOptionDto> available_commands;
    std::vector<std::string> warning_keys;
    std::vector<std::string> debug_trace_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// 13. Area DTOs (stub for future)
// ---------------------------------------------------------------------------

struct AreaShapeDto {
    AreaShapeKind shape_kind = AreaShapeKind::SingleCell;
    WorldCoordinateDto origin;
    int radius = 0;
    int width = 0;
    int height = 0;
    std::vector<WorldCoordinateDto> cells;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// ---------------------------------------------------------------------------
// Execution context used inside the pipeline
// ---------------------------------------------------------------------------

struct WorldCommandExecutionResult {
    WorldCommandResultKind result_kind = WorldCommandResultKind::Unknown;
    std::vector<std::string> failure_reason_keys;
    std::vector<std::string> warning_keys;
    std::vector<WorldStateDeltaDto> state_deltas;
    std::vector<WorldCommandDto> spawned_commands;
    std::vector<WorldEventDto> events;
    std::vector<WorldExperienceDto> experiences;
    std::vector<WorldFrontendHintDto> frontend_hints;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class WorldCommandContext {
public:
    explicit WorldCommandContext(const WorldCommandDto& command);

    const WorldCommandDto& command() const;
    uint64_t currentTick() const;
    void setCurrentTick(uint64_t tick);

    const std::vector<std::string>& traceStages() const;
    void addTraceStage(const std::string& stage);

private:
    WorldCommandDto command_;
    uint64_t current_tick_ = 0;
    std::vector<std::string> trace_stages_;
};

} // namespace pathfinder::world_command
