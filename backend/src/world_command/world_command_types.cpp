#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_command {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// WorldCommandKind
// ---------------------------------------------------------------------------
std::string toString(WorldCommandKind kind) {
    switch (kind) {
        case WorldCommandKind::Unknown:           return "unknown";
        case WorldCommandKind::Noop:              return "noop";
        case WorldCommandKind::Wait:              return "wait";
        case WorldCommandKind::Inspect:           return "inspect";
        case WorldCommandKind::Move:              return "move";
        case WorldCommandKind::Gather:            return "gather";
        case WorldCommandKind::Chop:              return "chop";
        case WorldCommandKind::Mine:              return "mine";
        case WorldCommandKind::Dig:               return "dig";
        case WorldCommandKind::Pickup:            return "pickup";
        case WorldCommandKind::Drop:              return "drop";
        case WorldCommandKind::Eat:               return "eat";
        case WorldCommandKind::Use:               return "use";
        case WorldCommandKind::Craft:             return "craft";
        case WorldCommandKind::Teach:             return "teach";
        case WorldCommandKind::Attack:            return "attack";
        case WorldCommandKind::Flee:              return "flee";
        case WorldCommandKind::GenerateWorld:     return "generate_world";
        case WorldCommandKind::GenerateRegion:    return "generate_region";
        case WorldCommandKind::PaintTerrain:       return "paint_terrain";
        case WorldCommandKind::SpawnEntity:       return "spawn_entity";
        case WorldCommandKind::DespawnEntity:     return "despawn_entity";
        case WorldCommandKind::TriggerAreaEvent:  return "trigger_area_event";
        case WorldCommandKind::ApplyAreaEffectTick: return "apply_area_effect_tick";
        case WorldCommandKind::CastAreaEffect:    return "cast_area_effect";
        case WorldCommandKind::SystemTick:        return "system_tick";
        default: return "unknown";
    }
}

Result<WorldCommandKind> worldCommandKindFromString(const std::string& str) {
    if (str == "noop")               return Result<WorldCommandKind>::ok(WorldCommandKind::Noop);
    if (str == "wait")               return Result<WorldCommandKind>::ok(WorldCommandKind::Wait);
    if (str == "inspect")            return Result<WorldCommandKind>::ok(WorldCommandKind::Inspect);
    if (str == "move")               return Result<WorldCommandKind>::ok(WorldCommandKind::Move);
    if (str == "gather")             return Result<WorldCommandKind>::ok(WorldCommandKind::Gather);
    if (str == "chop")               return Result<WorldCommandKind>::ok(WorldCommandKind::Chop);
    if (str == "mine")               return Result<WorldCommandKind>::ok(WorldCommandKind::Mine);
    if (str == "dig")                return Result<WorldCommandKind>::ok(WorldCommandKind::Dig);
    if (str == "pickup")             return Result<WorldCommandKind>::ok(WorldCommandKind::Pickup);
    if (str == "drop")               return Result<WorldCommandKind>::ok(WorldCommandKind::Drop);
    if (str == "eat")                return Result<WorldCommandKind>::ok(WorldCommandKind::Eat);
    if (str == "use")                return Result<WorldCommandKind>::ok(WorldCommandKind::Use);
    if (str == "craft")              return Result<WorldCommandKind>::ok(WorldCommandKind::Craft);
    if (str == "teach")              return Result<WorldCommandKind>::ok(WorldCommandKind::Teach);
    if (str == "attack")             return Result<WorldCommandKind>::ok(WorldCommandKind::Attack);
    if (str == "flee")               return Result<WorldCommandKind>::ok(WorldCommandKind::Flee);
    if (str == "generate_world")     return Result<WorldCommandKind>::ok(WorldCommandKind::GenerateWorld);
    if (str == "generate_region")    return Result<WorldCommandKind>::ok(WorldCommandKind::GenerateRegion);
    if (str == "paint_terrain")      return Result<WorldCommandKind>::ok(WorldCommandKind::PaintTerrain);
    if (str == "spawn_entity")       return Result<WorldCommandKind>::ok(WorldCommandKind::SpawnEntity);
    if (str == "despawn_entity")     return Result<WorldCommandKind>::ok(WorldCommandKind::DespawnEntity);
    if (str == "trigger_area_event") return Result<WorldCommandKind>::ok(WorldCommandKind::TriggerAreaEvent);
    if (str == "apply_area_effect_tick") return Result<WorldCommandKind>::ok(WorldCommandKind::ApplyAreaEffectTick);
    if (str == "cast_area_effect")   return Result<WorldCommandKind>::ok(WorldCommandKind::CastAreaEffect);
    if (str == "system_tick")        return Result<WorldCommandKind>::ok(WorldCommandKind::SystemTick);
    return Result<WorldCommandKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown WorldCommandKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// WorldCommandSource
// ---------------------------------------------------------------------------
std::string toString(WorldCommandSource source) {
    switch (source) {
        case WorldCommandSource::Unknown:          return "unknown";
        case WorldCommandSource::PlayerInput:      return "player_input";
        case WorldCommandSource::AgentDecision:    return "agent_decision";
        case WorldCommandSource::BeastDecision:    return "beast_decision";
        case WorldCommandSource::SystemTick:       return "system_tick";
        case WorldCommandSource::WorldGeneration:  return "world_generation";
        case WorldCommandSource::AreaEffectTick:   return "area_effect_tick";
        case WorldCommandSource::ReactionChain:    return "reaction_chain";
        case WorldCommandSource::ScriptedScenario: return "scripted_scenario";
        case WorldCommandSource::Replay:           return "replay";
        case WorldCommandSource::Test:             return "test";
        default: return "unknown";
    }
}

Result<WorldCommandSource> worldCommandSourceFromString(const std::string& str) {
    if (str == "player_input")      return Result<WorldCommandSource>::ok(WorldCommandSource::PlayerInput);
    if (str == "agent_decision")    return Result<WorldCommandSource>::ok(WorldCommandSource::AgentDecision);
    if (str == "beast_decision")    return Result<WorldCommandSource>::ok(WorldCommandSource::BeastDecision);
    if (str == "system_tick")       return Result<WorldCommandSource>::ok(WorldCommandSource::SystemTick);
    if (str == "world_generation")  return Result<WorldCommandSource>::ok(WorldCommandSource::WorldGeneration);
    if (str == "area_effect_tick")  return Result<WorldCommandSource>::ok(WorldCommandSource::AreaEffectTick);
    if (str == "reaction_chain")    return Result<WorldCommandSource>::ok(WorldCommandSource::ReactionChain);
    if (str == "scripted_scenario") return Result<WorldCommandSource>::ok(WorldCommandSource::ScriptedScenario);
    if (str == "replay")            return Result<WorldCommandSource>::ok(WorldCommandSource::Replay);
    if (str == "test")              return Result<WorldCommandSource>::ok(WorldCommandSource::Test);
    return Result<WorldCommandSource>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown WorldCommandSource: " + str)
    );
}

// ---------------------------------------------------------------------------
// WorldCommandResultKind
// ---------------------------------------------------------------------------
std::string toString(WorldCommandResultKind kind) {
    switch (kind) {
        case WorldCommandResultKind::Unknown:     return "unknown";
        case WorldCommandResultKind::Succeeded:   return "succeeded";
        case WorldCommandResultKind::Failed:      return "failed";
        case WorldCommandResultKind::Blocked:     return "blocked";
        case WorldCommandResultKind::Interrupted: return "interrupted";
        case WorldCommandResultKind::Deferred:    return "deferred";
        case WorldCommandResultKind::Noop:        return "noop";
        default: return "unknown";
    }
}

Result<WorldCommandResultKind> worldCommandResultKindFromString(const std::string& str) {
    if (str == "succeeded")   return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Succeeded);
    if (str == "failed")      return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Failed);
    if (str == "blocked")     return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Blocked);
    if (str == "interrupted") return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Interrupted);
    if (str == "deferred")    return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Deferred);
    if (str == "noop")        return Result<WorldCommandResultKind>::ok(WorldCommandResultKind::Noop);
    return Result<WorldCommandResultKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown WorldCommandResultKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// WorldCommandTargetKind
// ---------------------------------------------------------------------------
std::string toString(WorldCommandTargetKind kind) {
    switch (kind) {
        case WorldCommandTargetKind::None:       return "none";
        case WorldCommandTargetKind::Coordinate: return "coordinate";
        case WorldCommandTargetKind::Entity:     return "entity";
        case WorldCommandTargetKind::Actor:      return "actor";
        case WorldCommandTargetKind::Item:       return "item";
        case WorldCommandTargetKind::Inventory:  return "inventory";
        case WorldCommandTargetKind::Area:       return "area";
        case WorldCommandTargetKind::Knowledge:  return "knowledge";
        case WorldCommandTargetKind::Recipe:     return "recipe";
        default: return "none";
    }
}

Result<WorldCommandTargetKind> worldCommandTargetKindFromString(const std::string& str) {
    if (str == "none")       return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::None);
    if (str == "coordinate") return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Coordinate);
    if (str == "entity")     return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Entity);
    if (str == "actor")      return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Actor);
    if (str == "item")       return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Item);
    if (str == "inventory")  return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Inventory);
    if (str == "area")       return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Area);
    if (str == "knowledge")  return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Knowledge);
    if (str == "recipe")     return Result<WorldCommandTargetKind>::ok(WorldCommandTargetKind::Recipe);
    return Result<WorldCommandTargetKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown WorldCommandTargetKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// PatchOp
// ---------------------------------------------------------------------------
std::string toString(PatchOp op) {
    switch (op) {
        case PatchOp::Add:     return "add";
        case PatchOp::Update:  return "update";
        case PatchOp::Remove:  return "remove";
        case PatchOp::Replace: return "replace";
        case PatchOp::Clear:   return "clear";
        default: return "update";
    }
}

Result<PatchOp> patchOpFromString(const std::string& str) {
    if (str == "add")     return Result<PatchOp>::ok(PatchOp::Add);
    if (str == "update")  return Result<PatchOp>::ok(PatchOp::Update);
    if (str == "remove")  return Result<PatchOp>::ok(PatchOp::Remove);
    if (str == "replace") return Result<PatchOp>::ok(PatchOp::Replace);
    if (str == "clear")   return Result<PatchOp>::ok(PatchOp::Clear);
    return Result<PatchOp>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown PatchOp: " + str)
    );
}

// ---------------------------------------------------------------------------
// FrontendHintKind
// ---------------------------------------------------------------------------
std::string toString(FrontendHintKind kind) {
    switch (kind) {
        case FrontendHintKind::HighlightCell:    return "highlight_cell";
        case FrontendHintKind::HighlightEntity:  return "highlight_entity";
        case FrontendHintKind::FloatingText:     return "floating_text";
        case FrontendHintKind::ShakeCell:        return "shake_cell";
        case FrontendHintKind::PlaySound:        return "play_sound";
        case FrontendHintKind::SpawnEffect:      return "spawn_effect";
        case FrontendHintKind::RemoveEffect:     return "remove_effect";
        case FrontendHintKind::OpenPanel:        return "open_panel";
        case FrontendHintKind::UpdateCommandBar: return "update_command_bar";
        case FrontendHintKind::FocusCamera:      return "focus_camera";
        default: return "floating_text";
    }
}

Result<FrontendHintKind> frontendHintKindFromString(const std::string& str) {
    if (str == "highlight_cell")    return Result<FrontendHintKind>::ok(FrontendHintKind::HighlightCell);
    if (str == "highlight_entity")  return Result<FrontendHintKind>::ok(FrontendHintKind::HighlightEntity);
    if (str == "floating_text")     return Result<FrontendHintKind>::ok(FrontendHintKind::FloatingText);
    if (str == "shake_cell")        return Result<FrontendHintKind>::ok(FrontendHintKind::ShakeCell);
    if (str == "play_sound")        return Result<FrontendHintKind>::ok(FrontendHintKind::PlaySound);
    if (str == "spawn_effect")      return Result<FrontendHintKind>::ok(FrontendHintKind::SpawnEffect);
    if (str == "remove_effect")     return Result<FrontendHintKind>::ok(FrontendHintKind::RemoveEffect);
    if (str == "open_panel")        return Result<FrontendHintKind>::ok(FrontendHintKind::OpenPanel);
    if (str == "update_command_bar") return Result<FrontendHintKind>::ok(FrontendHintKind::UpdateCommandBar);
    if (str == "focus_camera")      return Result<FrontendHintKind>::ok(FrontendHintKind::FocusCamera);
    return Result<FrontendHintKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown FrontendHintKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// AreaShapeKind
// ---------------------------------------------------------------------------
std::string toString(AreaShapeKind kind) {
    switch (kind) {
        case AreaShapeKind::SingleCell:  return "single_cell";
        case AreaShapeKind::CircleRadius:return "circle_radius";
        case AreaShapeKind::Rectangle:   return "rectangle";
        case AreaShapeKind::Line:        return "line";
        case AreaShapeKind::Cone:        return "cone";
        case AreaShapeKind::CustomMask:  return "custom_mask";
        default: return "single_cell";
    }
}

Result<AreaShapeKind> areaShapeKindFromString(const std::string& str) {
    if (str == "single_cell")   return Result<AreaShapeKind>::ok(AreaShapeKind::SingleCell);
    if (str == "circle_radius") return Result<AreaShapeKind>::ok(AreaShapeKind::CircleRadius);
    if (str == "rectangle")     return Result<AreaShapeKind>::ok(AreaShapeKind::Rectangle);
    if (str == "line")          return Result<AreaShapeKind>::ok(AreaShapeKind::Line);
    if (str == "cone")          return Result<AreaShapeKind>::ok(AreaShapeKind::Cone);
    if (str == "custom_mask")   return Result<AreaShapeKind>::ok(AreaShapeKind::CustomMask);
    return Result<AreaShapeKind>::fail(
        makeError(ErrorCode::validation_enum_unknown, "Unknown AreaShapeKind: " + str)
    );
}

// ---------------------------------------------------------------------------
// DTO validateBasic implementations
// ---------------------------------------------------------------------------
Result<void> WorldCoordinateDto::validateBasic() const {
    if (layer_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCoordinateDto.layer_key cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldCommandTargetDto::validateBasic() const {
    if (target_kind == WorldCommandTargetKind::Coordinate && !target_coord.has_value()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandTargetDto.target_coord required when target_kind is Coordinate")
        );
    }
    if (target_coord.has_value()) {
        auto coord_result = target_coord->validateBasic();
        if (coord_result.is_error()) {
            return coord_result;
        }
    }
    return Result<void>::ok();
}

Result<void> WorldCommandContextDto::validateBasic() const {
    return Result<void>::ok();
}

Result<void> WorldCommandDto::validateBasic() const {
    if (command_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandDto.command_id cannot be empty")
        );
    }
    if (command_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandDto.command_key cannot be empty")
        );
    }
    if (source == WorldCommandSource::Unknown) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandDto.source cannot be Unknown")
        );
    }
    if (source == WorldCommandSource::PlayerInput && actor_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandDto.actor_key required when source is PlayerInput")
        );
    }
    auto target_result = target.validateBasic();
    if (target_result.is_error()) {
        return target_result;
    }
    auto context_result = context.validateBasic();
    if (context_result.is_error()) {
        return context_result;
    }
    return Result<void>::ok();
}

Result<void> WorldStateDeltaDto::validateBasic() const {
    if (delta_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldStateDeltaDto.delta_id cannot be empty")
        );
    }
    if (delta_kind.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldStateDeltaDto.delta_kind cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldEventDto::validateBasic() const {
    if (event_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldEventDto.event_id cannot be empty")
        );
    }
    if (event_kind.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldEventDto.event_kind cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldExperienceDto::validateBasic() const {
    if (experience_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldExperienceDto.experience_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldCommandResultDto::validateBasic() const {
    if (command_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandResultDto.command_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldCellPatchDto::validateBasic() const {
    if (cell_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCellPatchDto.cell_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldEntityPatchDto::validateBasic() const {
    if (entity_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldEntityPatchDto.entity_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> InventoryPatchDto::validateBasic() const {
    if (inventory_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "InventoryPatchDto.inventory_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> KnowledgePatchDto::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "KnowledgePatchDto.actor_key cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> AreaEffectPatchDto::validateBasic() const {
    if (area_effect_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "AreaEffectPatchDto.area_effect_id cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldProjectionPatchDto::validateBasic() const {
    return Result<void>::ok();
}

Result<void> WorldFrontendHintDto::validateBasic() const {
    return Result<void>::ok();
}

Result<void> WorldCommandOptionDto::validateBasic() const {
    if (option_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandOptionDto.option_id cannot be empty")
        );
    }
    if (command_key.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandOptionDto.command_key cannot be empty")
        );
    }
    return Result<void>::ok();
}

Result<void> WorldCommandResponseDto::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandResponseDto.session_id cannot be empty")
        );
    }
    if (command_id.empty()) {
        return Result<void>::fail(
            makeError(ErrorCode::command_missing_required_field, "WorldCommandResponseDto.command_id cannot be empty")
        );
    }
    auto result_validation = result.validateBasic();
    if (result_validation.is_error()) {
        return result_validation;
    }
    auto patch_validation = projection_patch.validateBasic();
    if (patch_validation.is_error()) {
        return patch_validation;
    }
    return Result<void>::ok();
}

Result<void> AreaShapeDto::validateBasic() const {
    auto origin_result = origin.validateBasic();
    if (origin_result.is_error()) {
        return origin_result;
    }
    return Result<void>::ok();
}

Result<void> WorldCommandExecutionResult::validateBasic() const {
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// WorldCommandContext
// ---------------------------------------------------------------------------
WorldCommandContext::WorldCommandContext(const WorldCommandDto& command)
    : command_(command) {}

const WorldCommandDto& WorldCommandContext::command() const {
    return command_;
}

uint64_t WorldCommandContext::currentTick() const {
    return current_tick_;
}

void WorldCommandContext::setCurrentTick(uint64_t tick) {
    current_tick_ = tick;
}

const std::vector<std::string>& WorldCommandContext::traceStages() const {
    return trace_stages_;
}

void WorldCommandContext::addTraceStage(const std::string& stage) {
    trace_stages_.push_back(stage);
}

} // namespace pathfinder::world_command
