#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_map_interaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// MapSelectionKind
// ---------------------------------------------------------------------------

std::string toString(MapSelectionKind kind) {
    switch (kind) {
        case MapSelectionKind::Cell:          return "Cell";
        case MapSelectionKind::Entity:        return "Entity";
        case MapSelectionKind::ResourceNode:  return "ResourceNode";
        case MapSelectionKind::GroundItem:    return "GroundItem";
        case MapSelectionKind::InventoryItem: return "InventoryItem";
        case MapSelectionKind::TestOnly:      return "TestOnly";
        default:                              return "Unknown";
    }
}

Result<MapSelectionKind> mapSelectionKindFromString(const std::string& str) {
    if (str == "Cell")          return Result<MapSelectionKind>::ok(MapSelectionKind::Cell);
    if (str == "Entity")        return Result<MapSelectionKind>::ok(MapSelectionKind::Entity);
    if (str == "ResourceNode")  return Result<MapSelectionKind>::ok(MapSelectionKind::ResourceNode);
    if (str == "GroundItem")    return Result<MapSelectionKind>::ok(MapSelectionKind::GroundItem);
    if (str == "InventoryItem") return Result<MapSelectionKind>::ok(MapSelectionKind::InventoryItem);
    if (str == "TestOnly")      return Result<MapSelectionKind>::ok(MapSelectionKind::TestOnly);
    return Result<MapSelectionKind>::fail(makeError(ErrorCode::validation_enum_unknown,
        "Unknown MapSelectionKind: " + str));
}

// ---------------------------------------------------------------------------
// MapActionOptionStatus
// ---------------------------------------------------------------------------

std::string toString(MapActionOptionStatus status) {
    switch (status) {
        case MapActionOptionStatus::Available: return "Available";
        case MapActionOptionStatus::Disabled:  return "Disabled";
        case MapActionOptionStatus::Hidden:    return "Hidden";
        case MapActionOptionStatus::TestOnly:  return "TestOnly";
        default:                               return "Unknown";
    }
}

Result<MapActionOptionStatus> mapActionOptionStatusFromString(const std::string& str) {
    if (str == "Available") return Result<MapActionOptionStatus>::ok(MapActionOptionStatus::Available);
    if (str == "Disabled")  return Result<MapActionOptionStatus>::ok(MapActionOptionStatus::Disabled);
    if (str == "Hidden")    return Result<MapActionOptionStatus>::ok(MapActionOptionStatus::Hidden);
    if (str == "TestOnly")  return Result<MapActionOptionStatus>::ok(MapActionOptionStatus::TestOnly);
    return Result<MapActionOptionStatus>::fail(makeError(ErrorCode::validation_enum_unknown,
        "Unknown MapActionOptionStatus: " + str));
}

// ---------------------------------------------------------------------------
// RegionActivityWindow validateBasic
// ---------------------------------------------------------------------------

Result<void> RegionActivityWindow::validateBasic() const {
    if (world_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "world_id is empty"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "layer_key is empty"));
    }
    if (center_actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "center_actor_key is empty"));
    }
    if (vision_radius < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "vision_radius is negative"));
    }
    if (preload_margin_regions < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "preload_margin_regions is negative"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapInteractionContext validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapInteractionContext::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id is empty"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key is empty"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "layer_key is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapSelectionRequest validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapSelectionRequest::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id is empty"));
    }
    if (client_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "client_id is empty"));
    }
    if (selection_kind == MapSelectionKind::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "selection_kind is Unknown"));
    }
    if (selection_kind == MapSelectionKind::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "selection_kind is TestOnly"));
    }
    if (layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "layer_key is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapSelectedObjectSummary validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapSelectedObjectSummary::validateBasic() const {
    if (object_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "object_id is empty"));
    }
    if (object_kind.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "object_kind is empty"));
    }
    if (display_name_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "display_name_key is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapSelectionResponse validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapSelectionResponse::validateBasic() const {
    if (session_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "session_id is empty"));
    }
    if (valid && selection_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "selection_id is empty for valid selection"));
    }
    for (const auto& obj : selected_objects) {
        auto res = obj.validateBasic();
        if (res.is_error()) return res;
    }
    for (const auto& opt : available_options) {
        auto res = opt.validateBasic();
        if (res.is_error()) return res;
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapActionOptionDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapActionOptionDto::validateBasic() const {
    if (option_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "option_id is empty"));
    }
    if (command_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "command_key is empty"));
    }
    if (status == MapActionOptionStatus::Unknown) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "status is Unknown"));
    }
    if (status == MapActionOptionStatus::TestOnly) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "status is TestOnly"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapPathPreviewDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapPathPreviewDto::validateBasic() const {
    if (reachable && estimated_steps < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "estimated_steps is negative"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientRegionLifecycleHintDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientRegionLifecycleHintDto::validateBasic() const {
    if (region_runtime_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "region_runtime_id is empty"));
    }
    if (hint_kind.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "hint_kind is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientRegionLifecycleEventDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientRegionLifecycleEventDto::validateBasic() const {
    if (event_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "event_id is empty"));
    }
    if (region_runtime_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "region_runtime_id is empty"));
    }
    if (event_kind.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "event_kind is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientKnowledgeFeedbackDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientKnowledgeFeedbackDto::validateBasic() const {
    if (feedback_id.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "feedback_id is empty"));
    }
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key is empty"));
    }
    if (label_zh.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "label_zh is empty"));
    }
    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ClientMapProjectionDto validateBasic
// ---------------------------------------------------------------------------

Result<void> ClientMapProjectionDto::validateBasic() const {
    if (actor_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "actor_key is empty"));
    }
    if (active_layer_key.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "active_layer_key is empty"));
    }
    if (viewport_radius < 0) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "viewport_radius is negative"));
    }
    for (const auto& cell : visible_cells) {
        auto res = cell.validateBasic();
        if (res.is_error()) return res;
    }
    for (const auto& ent : visible_entities) {
        auto res = ent.validateBasic();
        if (res.is_error()) return res;
    }
    for (const auto& hint : region_lifecycle_hints) {
        auto res = hint.validateBasic();
        if (res.is_error()) return res;
    }
    return Result<void>::ok();
}

} // namespace pathfinder::world_map_interaction
