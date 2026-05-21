#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/foundation/error.h"
#include "json.hpp"

namespace pathfinder::client_protocol {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using nlohmann::json;

namespace {

using pathfinder::world_command::worldCommandTargetKindFromString;
using pathfinder::world_command::worldCommandKindFromString;
using pathfinder::world_command::worldCommandSourceFromString;
using pathfinder::world_command::frontendHintKindFromString;
using pathfinder::world_command::worldCommandResultKindFromString;
using pathfinder::world_command::patchOpFromString;
using pathfinder::client_protocol::clientSubmitModeFromString;
using pathfinder::client_protocol::clientProjectionScopeFromString;

// ---------------------------------------------------------------------------
// Manual encode/decode helpers for world_command types
// ---------------------------------------------------------------------------

json encodeCoordinate(const WorldCoordinateDto& v) {
    return json{{"x", v.x}, {"y", v.y}, {"layer_key", v.layer_key}};
}

void decodeCoordinate(const json& j, WorldCoordinateDto& v) {
    if (j.contains("x")) j.at("x").get_to(v.x);
    if (j.contains("y")) j.at("y").get_to(v.y);
    if (j.contains("layer_key")) j.at("layer_key").get_to(v.layer_key);
}

json encodeTarget(const WorldCommandTargetDto& v) {
    json j = {
        {"target_kind", toString(v.target_kind)},
        {"target_entity_id", v.target_entity_id},
        {"target_actor_key", v.target_actor_key},
        {"target_item_key", v.target_item_key},
        {"target_inventory_id", v.target_inventory_id},
        {"target_area_id", v.target_area_id},
        {"knowledge_key", v.knowledge_key},
        {"recipe_key", v.recipe_key}
    };
    if (v.target_coord.has_value()) {
        j["target_coord"] = encodeCoordinate(v.target_coord.value());
    } else {
        j["target_coord"] = nullptr;
    }
    return j;
}

void decodeTarget(const json& j, WorldCommandTargetDto& v) {
    if (j.contains("target_kind")) {
        auto r = worldCommandTargetKindFromString(j.at("target_kind").get<std::string>());
        if (r.is_ok()) v.target_kind = r.value();
    }
    if (j.contains("target_coord") && !j.at("target_coord").is_null()) {
        WorldCoordinateDto coord;
        decodeCoordinate(j.at("target_coord"), coord);
        v.target_coord = coord;
    }
    if (j.contains("target_entity_id")) j.at("target_entity_id").get_to(v.target_entity_id);
    if (j.contains("target_actor_key")) j.at("target_actor_key").get_to(v.target_actor_key);
    if (j.contains("target_item_key")) j.at("target_item_key").get_to(v.target_item_key);
    if (j.contains("target_inventory_id")) j.at("target_inventory_id").get_to(v.target_inventory_id);
    if (j.contains("target_area_id")) j.at("target_area_id").get_to(v.target_area_id);
    if (j.contains("knowledge_key")) j.at("knowledge_key").get_to(v.knowledge_key);
    if (j.contains("recipe_key")) j.at("recipe_key").get_to(v.recipe_key);
}

json encodeContext(const WorldCommandContextDto& v) {
    return json{
        {"issued_tick", v.issued_tick},
        {"client_sequence", v.client_sequence},
        {"correlation_id", v.correlation_id},
        {"parent_command_id", v.parent_command_id},
        {"actor_location_layer", v.actor_location_layer},
        {"allow_hidden_debug", v.allow_hidden_debug}
    };
}

void decodeContext(const json& j, WorldCommandContextDto& v) {
    if (j.contains("issued_tick")) j.at("issued_tick").get_to(v.issued_tick);
    if (j.contains("client_sequence")) j.at("client_sequence").get_to(v.client_sequence);
    if (j.contains("correlation_id")) j.at("correlation_id").get_to(v.correlation_id);
    if (j.contains("parent_command_id")) j.at("parent_command_id").get_to(v.parent_command_id);
    if (j.contains("actor_location_layer")) j.at("actor_location_layer").get_to(v.actor_location_layer);
    if (j.contains("allow_hidden_debug")) j.at("allow_hidden_debug").get_to(v.allow_hidden_debug);
}

json encodeCommand(const WorldCommandDto& v) {
    return json{
        {"command_id", v.command_id},
        {"command_kind", toString(v.command_kind)},
        {"command_key", v.command_key},
        {"source", toString(v.source)},
        {"actor_key", v.actor_key},
        {"target", encodeTarget(v.target)},
        {"context", encodeContext(v.context)},
        {"parameters", v.parameters}
    };
}

void decodeCommand(const json& j, WorldCommandDto& v) {
    if (j.contains("command_id")) j.at("command_id").get_to(v.command_id);
    if (j.contains("command_kind")) {
        auto r = worldCommandKindFromString(j.at("command_kind").get<std::string>());
        if (r.is_ok()) v.command_kind = r.value();
    }
    if (j.contains("command_key")) j.at("command_key").get_to(v.command_key);
    if (j.contains("source")) {
        auto r = worldCommandSourceFromString(j.at("source").get<std::string>());
        if (r.is_ok()) v.source = r.value();
    }
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("target")) decodeTarget(j.at("target"), v.target);
    if (j.contains("context")) decodeContext(j.at("context"), v.context);
    if (j.contains("parameters")) j.at("parameters").get_to(v.parameters);
}

json encodeEvent(const WorldEventDto& v) {
    json j = {
        {"event_id", v.event_id},
        {"event_kind", v.event_kind},
        {"tick", v.tick},
        {"title_text", v.title_text},
        {"body_text", v.body_text},
        {"actor_key", v.actor_key},
        {"target_entity_id", v.target_entity_id},
        {"priority", v.priority},
        {"reason_keys", v.reason_keys}
    };
    if (v.coord.has_value()) {
        j["coord"] = encodeCoordinate(v.coord.value());
    } else {
        j["coord"] = nullptr;
    }
    return j;
}

void decodeEvent(const json& j, WorldEventDto& v) {
    if (j.contains("event_id")) j.at("event_id").get_to(v.event_id);
    if (j.contains("event_kind")) j.at("event_kind").get_to(v.event_kind);
    if (j.contains("tick")) j.at("tick").get_to(v.tick);
    if (j.contains("title_text")) j.at("title_text").get_to(v.title_text);
    if (j.contains("body_text")) j.at("body_text").get_to(v.body_text);
    if (j.contains("coord") && !j.at("coord").is_null()) {
        WorldCoordinateDto coord;
        decodeCoordinate(j.at("coord"), coord);
        v.coord = coord;
    }
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("target_entity_id")) j.at("target_entity_id").get_to(v.target_entity_id);
    if (j.contains("priority")) j.at("priority").get_to(v.priority);
    if (j.contains("reason_keys")) j.at("reason_keys").get_to(v.reason_keys);
}

json encodeExperience(const WorldExperienceDto& v) {
    return json{
        {"experience_id", v.experience_id},
        {"actor_key", v.actor_key},
        {"command_key", v.command_key},
        {"subject_entity_key", v.subject_entity_key},
        {"target_entity_key", v.target_entity_key},
        {"effect_key", v.effect_key},
        {"tick", v.tick},
        {"condition_keys", v.condition_keys},
        {"reason_keys", v.reason_keys}
    };
}

void decodeExperience(const json& j, WorldExperienceDto& v) {
    if (j.contains("experience_id")) j.at("experience_id").get_to(v.experience_id);
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("command_key")) j.at("command_key").get_to(v.command_key);
    if (j.contains("subject_entity_key")) j.at("subject_entity_key").get_to(v.subject_entity_key);
    if (j.contains("target_entity_key")) j.at("target_entity_key").get_to(v.target_entity_key);
    if (j.contains("effect_key")) j.at("effect_key").get_to(v.effect_key);
    if (j.contains("tick")) j.at("tick").get_to(v.tick);
    if (j.contains("condition_keys")) j.at("condition_keys").get_to(v.condition_keys);
    if (j.contains("reason_keys")) j.at("reason_keys").get_to(v.reason_keys);
}

json encodeHint(const WorldFrontendHintDto& v) {
    json j = {
        {"hint_kind", toString(v.hint_kind)},
        {"target_entity_id", v.target_entity_id},
        {"text", v.text},
        {"effect_key", v.effect_key},
        {"priority", v.priority},
        {"duration_ms", v.duration_ms}
    };
    if (v.target_coord.has_value()) {
        j["target_coord"] = encodeCoordinate(v.target_coord.value());
    } else {
        j["target_coord"] = nullptr;
    }
    return j;
}

void decodeHint(const json& j, WorldFrontendHintDto& v) {
    if (j.contains("hint_kind")) {
        auto r = frontendHintKindFromString(j.at("hint_kind").get<std::string>());
        if (r.is_ok()) v.hint_kind = r.value();
    }
    if (j.contains("target_entity_id")) j.at("target_entity_id").get_to(v.target_entity_id);
    if (j.contains("target_coord") && !j.at("target_coord").is_null()) {
        WorldCoordinateDto coord;
        decodeCoordinate(j.at("target_coord"), coord);
        v.target_coord = coord;
    }
    if (j.contains("text")) j.at("text").get_to(v.text);
    if (j.contains("effect_key")) j.at("effect_key").get_to(v.effect_key);
    if (j.contains("priority")) j.at("priority").get_to(v.priority);
    if (j.contains("duration_ms")) j.at("duration_ms").get_to(v.duration_ms);
}

json encodeOption(const WorldCommandOptionDto& v) {
    return json{
        {"option_id", v.option_id},
        {"command_kind", toString(v.command_kind)},
        {"command_key", v.command_key},
        {"label_text", v.label_text},
        {"target", encodeTarget(v.target)},
        {"enabled", v.enabled},
        {"disabled_reason_text", v.disabled_reason_text},
        {"priority", v.priority}
    };
}

void decodeOption(const json& j, WorldCommandOptionDto& v) {
    if (j.contains("option_id")) j.at("option_id").get_to(v.option_id);
    if (j.contains("command_kind")) {
        auto r = worldCommandKindFromString(j.at("command_kind").get<std::string>());
        if (r.is_ok()) v.command_kind = r.value();
    }
    if (j.contains("command_key")) j.at("command_key").get_to(v.command_key);
    if (j.contains("label_text")) j.at("label_text").get_to(v.label_text);
    if (j.contains("target")) decodeTarget(j.at("target"), v.target);
    if (j.contains("enabled")) j.at("enabled").get_to(v.enabled);
    if (j.contains("disabled_reason_text")) j.at("disabled_reason_text").get_to(v.disabled_reason_text);
    if (j.contains("priority")) j.at("priority").get_to(v.priority);
}

json encodeCellPatch(const WorldCellPatchDto& v) {
    return json{{"cell_id", v.cell_id}, {"op", toString(v.op)}, {"fields", v.fields}};
}

void decodeCellPatch(const json& j, WorldCellPatchDto& v) {
    if (j.contains("cell_id")) j.at("cell_id").get_to(v.cell_id);
    if (j.contains("op")) {
        auto r = patchOpFromString(j.at("op").get<std::string>());
        if (r.is_ok()) v.op = r.value();
    }
    if (j.contains("fields")) j.at("fields").get_to(v.fields);
}

json encodeEntityPatch(const WorldEntityPatchDto& v) {
    return json{{"entity_id", v.entity_id}, {"op", toString(v.op)}, {"fields", v.fields}};
}

void decodeEntityPatch(const json& j, WorldEntityPatchDto& v) {
    if (j.contains("entity_id")) j.at("entity_id").get_to(v.entity_id);
    if (j.contains("op")) {
        auto r = patchOpFromString(j.at("op").get<std::string>());
        if (r.is_ok()) v.op = r.value();
    }
    if (j.contains("fields")) j.at("fields").get_to(v.fields);
}

json encodeInventoryPatch(const InventoryPatchDto& v) {
    return json{{"inventory_id", v.inventory_id}, {"op", toString(v.op)}, {"fields", v.fields}};
}

void decodeInventoryPatch(const json& j, InventoryPatchDto& v) {
    if (j.contains("inventory_id")) j.at("inventory_id").get_to(v.inventory_id);
    if (j.contains("op")) {
        auto r = patchOpFromString(j.at("op").get<std::string>());
        if (r.is_ok()) v.op = r.value();
    }
    if (j.contains("fields")) j.at("fields").get_to(v.fields);
}

json encodeKnowledgePatch(const KnowledgePatchDto& v) {
    return json{{"actor_key", v.actor_key}, {"op", toString(v.op)}, {"fields", v.fields}};
}

void decodeKnowledgePatch(const json& j, KnowledgePatchDto& v) {
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("op")) {
        auto r = patchOpFromString(j.at("op").get<std::string>());
        if (r.is_ok()) v.op = r.value();
    }
    if (j.contains("fields")) j.at("fields").get_to(v.fields);
}

json encodeAreaEffectPatch(const AreaEffectPatchDto& v) {
    return json{{"area_effect_id", v.area_effect_id}, {"op", toString(v.op)}, {"fields", v.fields}};
}

void decodeAreaEffectPatch(const json& j, AreaEffectPatchDto& v) {
    if (j.contains("area_effect_id")) j.at("area_effect_id").get_to(v.area_effect_id);
    if (j.contains("op")) {
        auto r = patchOpFromString(j.at("op").get<std::string>());
        if (r.is_ok()) v.op = r.value();
    }
    if (j.contains("fields")) j.at("fields").get_to(v.fields);
}

json encodePatch(const WorldProjectionPatchDto& v) {
    json cells = json::array();
    for (const auto& c : v.changed_cells) cells.push_back(encodeCellPatch(c));
    json entities = json::array();
    for (const auto& e : v.changed_entities) entities.push_back(encodeEntityPatch(e));
    json inventories = json::array();
    for (const auto& i : v.changed_inventories) inventories.push_back(encodeInventoryPatch(i));
    json knowledge = json::array();
    for (const auto& k : v.changed_knowledge) knowledge.push_back(encodeKnowledgePatch(k));
    json area_effects = json::array();
    for (const auto& a : v.changed_area_effects) area_effects.push_back(encodeAreaEffectPatch(a));
    return json{
        {"base_projection_version", v.base_projection_version},
        {"new_projection_version", v.new_projection_version},
        {"requires_full_refresh", v.requires_full_refresh},
        {"changed_cells", cells},
        {"changed_entities", entities},
        {"changed_inventories", inventories},
        {"changed_knowledge", knowledge},
        {"changed_area_effects", area_effects}
    };
}

void decodePatch(const json& j, WorldProjectionPatchDto& v) {
    if (j.contains("base_projection_version")) j.at("base_projection_version").get_to(v.base_projection_version);
    if (j.contains("new_projection_version")) j.at("new_projection_version").get_to(v.new_projection_version);
    if (j.contains("requires_full_refresh")) j.at("requires_full_refresh").get_to(v.requires_full_refresh);
    if (j.contains("changed_cells")) {
        for (const auto& item : j.at("changed_cells")) {
            WorldCellPatchDto p;
            decodeCellPatch(item, p);
            v.changed_cells.push_back(p);
        }
    }
    if (j.contains("changed_entities")) {
        for (const auto& item : j.at("changed_entities")) {
            WorldEntityPatchDto p;
            decodeEntityPatch(item, p);
            v.changed_entities.push_back(p);
        }
    }
    if (j.contains("changed_inventories")) {
        for (const auto& item : j.at("changed_inventories")) {
            InventoryPatchDto p;
            decodeInventoryPatch(item, p);
            v.changed_inventories.push_back(p);
        }
    }
    if (j.contains("changed_knowledge")) {
        for (const auto& item : j.at("changed_knowledge")) {
            KnowledgePatchDto p;
            decodeKnowledgePatch(item, p);
            v.changed_knowledge.push_back(p);
        }
    }
    if (j.contains("changed_area_effects")) {
        for (const auto& item : j.at("changed_area_effects")) {
            AreaEffectPatchDto p;
            decodeAreaEffectPatch(item, p);
            v.changed_area_effects.push_back(p);
        }
    }
}

json encodeResult(const WorldCommandResultDto& v) {
    return json{
        {"command_id", v.command_id},
        {"command_key", v.command_key},
        {"result_kind", toString(v.result_kind)},
        {"actor_key", v.actor_key},
        {"target_ref", v.target_ref},
        {"failure_reason_keys", v.failure_reason_keys},
        {"warning_keys", v.warning_keys}
    };
}

void decodeResult(const json& j, WorldCommandResultDto& v) {
    if (j.contains("command_id")) j.at("command_id").get_to(v.command_id);
    if (j.contains("command_key")) j.at("command_key").get_to(v.command_key);
    if (j.contains("result_kind")) {
        auto r = worldCommandResultKindFromString(j.at("result_kind").get<std::string>());
        if (r.is_ok()) v.result_kind = r.value();
    }
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("target_ref")) j.at("target_ref").get_to(v.target_ref);
    if (j.contains("failure_reason_keys")) j.at("failure_reason_keys").get_to(v.failure_reason_keys);
    if (j.contains("warning_keys")) j.at("warning_keys").get_to(v.warning_keys);
}

// ---------------------------------------------------------------------------
// Client protocol DTO encode/decode
// ---------------------------------------------------------------------------

json encodeSelectionContext(const ClientSelectionContext& v) {
    return json{
        {"target", encodeTarget(v.target)},
        {"selected_actor_key", v.selected_actor_key},
        {"selected_entity_id", v.selected_entity_id},
        {"selected_inventory_id", v.selected_inventory_id},
        {"selected_area_id", v.selected_area_id}
    };
}

void decodeSelectionContext(const json& j, ClientSelectionContext& v) {
    if (j.contains("target")) decodeTarget(j.at("target"), v.target);
    if (j.contains("selected_actor_key")) j.at("selected_actor_key").get_to(v.selected_actor_key);
    if (j.contains("selected_entity_id")) j.at("selected_entity_id").get_to(v.selected_entity_id);
    if (j.contains("selected_inventory_id")) j.at("selected_inventory_id").get_to(v.selected_inventory_id);
    if (j.contains("selected_area_id")) j.at("selected_area_id").get_to(v.selected_area_id);
}

json encodeWorldProjection(const ClientWorldProjection& v) {
    json cells = json::array();
    for (const auto& c : v.visible_cells) cells.push_back(encodeCellPatch(c));
    json entities = json::array();
    for (const auto& e : v.visible_entities) entities.push_back(encodeEntityPatch(e));
    json inventories = json::array();
    for (const auto& i : v.inventories) inventories.push_back(encodeInventoryPatch(i));
    json knowledge = json::array();
    for (const auto& k : v.knowledge) knowledge.push_back(encodeKnowledgePatch(k));
    json area_effects = json::array();
    for (const auto& a : v.area_effects) area_effects.push_back(encodeAreaEffectPatch(a));
    return json{
        {"projection_version", v.projection_version},
        {"actor_key", v.actor_key},
        {"active_layer_key", v.active_layer_key},
        {"visible_cells", cells},
        {"visible_entities", entities},
        {"inventories", inventories},
        {"knowledge", knowledge},
        {"area_effects", area_effects},
        {"safe_summary_keys", v.safe_summary_keys}
    };
}

void decodeWorldProjection(const json& j, ClientWorldProjection& v) {
    if (j.contains("projection_version")) j.at("projection_version").get_to(v.projection_version);
    if (j.contains("actor_key")) j.at("actor_key").get_to(v.actor_key);
    if (j.contains("active_layer_key")) j.at("active_layer_key").get_to(v.active_layer_key);
    if (j.contains("visible_cells")) {
        for (const auto& item : j.at("visible_cells")) {
            WorldCellPatchDto p;
            decodeCellPatch(item, p);
            v.visible_cells.push_back(p);
        }
    }
    if (j.contains("visible_entities")) {
        for (const auto& item : j.at("visible_entities")) {
            WorldEntityPatchDto p;
            decodeEntityPatch(item, p);
            v.visible_entities.push_back(p);
        }
    }
    if (j.contains("inventories")) {
        for (const auto& item : j.at("inventories")) {
            InventoryPatchDto p;
            decodeInventoryPatch(item, p);
            v.inventories.push_back(p);
        }
    }
    if (j.contains("knowledge")) {
        for (const auto& item : j.at("knowledge")) {
            KnowledgePatchDto p;
            decodeKnowledgePatch(item, p);
            v.knowledge.push_back(p);
        }
    }
    if (j.contains("area_effects")) {
        for (const auto& item : j.at("area_effects")) {
            AreaEffectPatchDto p;
            decodeAreaEffectPatch(item, p);
            v.area_effects.push_back(p);
        }
    }
    if (j.contains("safe_summary_keys")) j.at("safe_summary_keys").get_to(v.safe_summary_keys);
}

json jsonEncodeBootstrapRequest(const ClientBootstrapRequest& v) {
    return json{
        {"client_id", v.client_id},
        {"session_id", v.session_id},
        {"requested_actor_key", v.requested_actor_key},
        {"requested_layer_key", v.requested_layer_key},
        {"client_protocol_version", v.client_protocol_version},
        {"create_if_missing", v.create_if_missing},
        {"dev_reset_if_allowed", v.dev_reset_if_allowed}
    };
}

void jsonDecodeBootstrapRequest(const json& j, ClientBootstrapRequest& v) {
    if (j.contains("client_id")) j.at("client_id").get_to(v.client_id);
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("requested_actor_key")) j.at("requested_actor_key").get_to(v.requested_actor_key);
    if (j.contains("requested_layer_key")) j.at("requested_layer_key").get_to(v.requested_layer_key);
    if (j.contains("client_protocol_version")) j.at("client_protocol_version").get_to(v.client_protocol_version);
    if (j.contains("create_if_missing")) j.at("create_if_missing").get_to(v.create_if_missing);
    if (j.contains("dev_reset_if_allowed")) j.at("dev_reset_if_allowed").get_to(v.dev_reset_if_allowed);
}

json jsonEncodeBootstrapResponse(const ClientBootstrapResponse& v) {
    json options = json::array();
    for (const auto& opt : v.available_commands) options.push_back(encodeOption(opt));
    json events = json::array();
    for (const auto& evt : v.event_feed) events.push_back(encodeEvent(evt));
    json hints = json::array();
    for (const auto& h : v.frontend_hints) hints.push_back(encodeHint(h));
    return json{
        {"session_id", v.session_id},
        {"server_protocol_version", v.server_protocol_version},
        {"projection_version", v.projection_version},
        {"full_projection", encodeWorldProjection(v.full_projection)},
        {"available_commands", options},
        {"event_feed", events},
        {"frontend_hints", hints},
        {"warning_keys", v.warning_keys}
    };
}

void jsonDecodeBootstrapResponse(const json& j, ClientBootstrapResponse& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("server_protocol_version")) j.at("server_protocol_version").get_to(v.server_protocol_version);
    if (j.contains("projection_version")) j.at("projection_version").get_to(v.projection_version);
    if (j.contains("full_projection")) decodeWorldProjection(j.at("full_projection"), v.full_projection);
    if (j.contains("available_commands")) {
        for (const auto& item : j.at("available_commands")) {
            WorldCommandOptionDto opt;
            decodeOption(item, opt);
            v.available_commands.push_back(opt);
        }
    }
    if (j.contains("event_feed")) {
        for (const auto& item : j.at("event_feed")) {
            WorldEventDto evt;
            decodeEvent(item, evt);
            v.event_feed.push_back(evt);
        }
    }
    if (j.contains("frontend_hints")) {
        for (const auto& item : j.at("frontend_hints")) {
            WorldFrontendHintDto h;
            decodeHint(item, h);
            v.frontend_hints.push_back(h);
        }
    }
    if (j.contains("warning_keys")) j.at("warning_keys").get_to(v.warning_keys);
}

json jsonEncodeCommandRequest(const ClientCommandRequest& v) {
    json j = {
        {"session_id", v.session_id},
        {"client_id", v.client_id},
        {"client_sequence", v.client_sequence},
        {"known_projection_version", v.known_projection_version},
        {"submit_mode", toString(v.submit_mode)},
        {"option_id", v.option_id},
        {"selection_context", encodeSelectionContext(v.selection_context)}
    };
    if (v.command.has_value()) {
        j["command"] = encodeCommand(v.command.value());
    } else {
        j["command"] = nullptr;
    }
    return j;
}

void jsonDecodeCommandRequest(const json& j, ClientCommandRequest& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("client_id")) j.at("client_id").get_to(v.client_id);
    if (j.contains("client_sequence")) j.at("client_sequence").get_to(v.client_sequence);
    if (j.contains("known_projection_version")) j.at("known_projection_version").get_to(v.known_projection_version);
    if (j.contains("submit_mode")) {
        auto r = clientSubmitModeFromString(j.at("submit_mode").get<std::string>());
        if (r.is_ok()) v.submit_mode = r.value();
    }
    if (j.contains("option_id")) j.at("option_id").get_to(v.option_id);
    if (j.contains("command") && !j.at("command").is_null()) {
        WorldCommandDto cmd;
        decodeCommand(j.at("command"), cmd);
        v.command = cmd;
    }
    if (j.contains("selection_context")) decodeSelectionContext(j.at("selection_context"), v.selection_context);
}

json jsonEncodeCommandResponse(const ClientCommandResponse& v) {
    json options = json::array();
    for (const auto& opt : v.available_commands) options.push_back(encodeOption(opt));
    json events = json::array();
    for (const auto& evt : v.event_feed) events.push_back(encodeEvent(evt));
    json exps = json::array();
    for (const auto& e : v.experiences) exps.push_back(encodeExperience(e));
    json hints = json::array();
    for (const auto& h : v.frontend_hints) hints.push_back(encodeHint(h));
    return json{
        {"session_id", v.session_id},
        {"accepted_client_sequence", v.accepted_client_sequence},
        {"base_projection_version", v.base_projection_version},
        {"new_projection_version", v.new_projection_version},
        {"requires_full_refresh", v.requires_full_refresh},
        {"result", encodeResult(v.result)},
        {"projection_patch", encodePatch(v.projection_patch)},
        {"available_commands", options},
        {"event_feed", events},
        {"experiences", exps},
        {"frontend_hints", hints},
        {"warning_keys", v.warning_keys}
    };
}

void jsonDecodeCommandResponse(const json& j, ClientCommandResponse& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("accepted_client_sequence")) j.at("accepted_client_sequence").get_to(v.accepted_client_sequence);
    if (j.contains("base_projection_version")) j.at("base_projection_version").get_to(v.base_projection_version);
    if (j.contains("new_projection_version")) j.at("new_projection_version").get_to(v.new_projection_version);
    if (j.contains("requires_full_refresh")) j.at("requires_full_refresh").get_to(v.requires_full_refresh);
    if (j.contains("result")) decodeResult(j.at("result"), v.result);
    if (j.contains("projection_patch")) decodePatch(j.at("projection_patch"), v.projection_patch);
    if (j.contains("available_commands")) {
        for (const auto& item : j.at("available_commands")) {
            WorldCommandOptionDto opt;
            decodeOption(item, opt);
            v.available_commands.push_back(opt);
        }
    }
    if (j.contains("event_feed")) {
        for (const auto& item : j.at("event_feed")) {
            WorldEventDto evt;
            decodeEvent(item, evt);
            v.event_feed.push_back(evt);
        }
    }
    if (j.contains("experiences")) {
        for (const auto& item : j.at("experiences")) {
            WorldExperienceDto e;
            decodeExperience(item, e);
            v.experiences.push_back(e);
        }
    }
    if (j.contains("frontend_hints")) {
        for (const auto& item : j.at("frontend_hints")) {
            WorldFrontendHintDto h;
            decodeHint(item, h);
            v.frontend_hints.push_back(h);
        }
    }
    if (j.contains("warning_keys")) j.at("warning_keys").get_to(v.warning_keys);
}

json jsonEncodeRefreshRequest(const ClientRefreshRequest& v) {
    json scopes = json::array();
    for (const auto& s : v.requested_scopes) scopes.push_back(toString(s));
    return json{
        {"session_id", v.session_id},
        {"client_id", v.client_id},
        {"known_projection_version", v.known_projection_version},
        {"requested_scopes", scopes},
        {"requested_layer_key", v.requested_layer_key},
        {"viewport_center_x", v.viewport_center_x},
        {"viewport_center_y", v.viewport_center_y}
    };
}

void jsonDecodeRefreshRequest(const json& j, ClientRefreshRequest& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("client_id")) j.at("client_id").get_to(v.client_id);
    if (j.contains("known_projection_version")) j.at("known_projection_version").get_to(v.known_projection_version);
    if (j.contains("requested_scopes") && j.at("requested_scopes").is_array()) {
        for (const auto& item : j.at("requested_scopes")) {
            auto r = clientProjectionScopeFromString(item.get<std::string>());
            if (r.is_ok()) v.requested_scopes.push_back(r.value());
        }
    }
    if (j.contains("requested_layer_key")) j.at("requested_layer_key").get_to(v.requested_layer_key);
    if (j.contains("viewport_center_x")) j.at("viewport_center_x").get_to(v.viewport_center_x);
    if (j.contains("viewport_center_y")) j.at("viewport_center_y").get_to(v.viewport_center_y);
}

json jsonEncodeRefreshResponse(const ClientRefreshResponse& v) {
    json options = json::array();
    for (const auto& opt : v.available_commands) options.push_back(encodeOption(opt));
    json events = json::array();
    for (const auto& evt : v.event_feed) events.push_back(encodeEvent(evt));
    json hints = json::array();
    for (const auto& h : v.frontend_hints) hints.push_back(encodeHint(h));
    return json{
        {"session_id", v.session_id},
        {"projection_version", v.projection_version},
        {"full_projection", encodeWorldProjection(v.full_projection)},
        {"available_commands", options},
        {"event_feed", events},
        {"frontend_hints", hints},
        {"warning_keys", v.warning_keys}
    };
}

void jsonDecodeRefreshResponse(const json& j, ClientRefreshResponse& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("projection_version")) j.at("projection_version").get_to(v.projection_version);
    if (j.contains("full_projection")) decodeWorldProjection(j.at("full_projection"), v.full_projection);
    if (j.contains("available_commands")) {
        for (const auto& item : j.at("available_commands")) {
            WorldCommandOptionDto opt;
            decodeOption(item, opt);
            v.available_commands.push_back(opt);
        }
    }
    if (j.contains("event_feed")) {
        for (const auto& item : j.at("event_feed")) {
            WorldEventDto evt;
            decodeEvent(item, evt);
            v.event_feed.push_back(evt);
        }
    }
    if (j.contains("frontend_hints")) {
        for (const auto& item : j.at("frontend_hints")) {
            WorldFrontendHintDto h;
            decodeHint(item, h);
            v.frontend_hints.push_back(h);
        }
    }
    if (j.contains("warning_keys")) j.at("warning_keys").get_to(v.warning_keys);
}

json jsonEncodeResetRequest(const ClientResetRequest& v) {
    return json{{"session_id", v.session_id}, {"client_id", v.client_id}, {"confirmed", v.confirmed}};
}

void jsonDecodeResetRequest(const json& j, ClientResetRequest& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("client_id")) j.at("client_id").get_to(v.client_id);
    if (j.contains("confirmed")) j.at("confirmed").get_to(v.confirmed);
}

json jsonEncodeResetResponse(const ClientResetResponse& v) {
    return json{{"session_id", v.session_id}, {"bootstrap", jsonEncodeBootstrapResponse(v.bootstrap)}};
}

void jsonDecodeResetResponse(const json& j, ClientResetResponse& v) {
    if (j.contains("session_id")) j.at("session_id").get_to(v.session_id);
    if (j.contains("bootstrap")) jsonDecodeBootstrapResponse(j.at("bootstrap"), v.bootstrap);
}

} // namespace

// ---------------------------------------------------------------------------
// Codec implementation
// ---------------------------------------------------------------------------

Result<std::string> ClientProtocolCodec::encodeBootstrapRequest(const ClientBootstrapRequest& request) const {
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeBootstrapRequest(request).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientBootstrapRequest> ClientProtocolCodec::decodeBootstrapRequest(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientBootstrapRequest request;
        jsonDecodeBootstrapRequest(j, request);
        auto valid = request.validateBasic();
        if (valid.is_error()) return Result<ClientBootstrapRequest>::fail(valid.errors());
        return Result<ClientBootstrapRequest>::ok(std::move(request));
    } catch (const std::exception& e) {
        return Result<ClientBootstrapRequest>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeBootstrapResponse(const ClientBootstrapResponse& response) const {
    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeBootstrapResponse(response).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientBootstrapResponse> ClientProtocolCodec::decodeBootstrapResponse(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientBootstrapResponse response;
        jsonDecodeBootstrapResponse(j, response);
        auto valid = response.validateBasic();
        if (valid.is_error()) return Result<ClientBootstrapResponse>::fail(valid.errors());
        return Result<ClientBootstrapResponse>::ok(std::move(response));
    } catch (const std::exception& e) {
        return Result<ClientBootstrapResponse>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeCommandRequest(const ClientCommandRequest& request) const {
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeCommandRequest(request).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientCommandRequest> ClientProtocolCodec::decodeCommandRequest(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientCommandRequest request;
        jsonDecodeCommandRequest(j, request);
        auto valid = request.validateBasic();
        if (valid.is_error()) return Result<ClientCommandRequest>::fail(valid.errors());
        return Result<ClientCommandRequest>::ok(std::move(request));
    } catch (const std::exception& e) {
        return Result<ClientCommandRequest>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeCommandResponse(const ClientCommandResponse& response) const {
    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeCommandResponse(response).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientCommandResponse> ClientProtocolCodec::decodeCommandResponse(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientCommandResponse response;
        jsonDecodeCommandResponse(j, response);
        auto valid = response.validateBasic();
        if (valid.is_error()) return Result<ClientCommandResponse>::fail(valid.errors());
        return Result<ClientCommandResponse>::ok(std::move(response));
    } catch (const std::exception& e) {
        return Result<ClientCommandResponse>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeRefreshRequest(const ClientRefreshRequest& request) const {
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeRefreshRequest(request).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientRefreshRequest> ClientProtocolCodec::decodeRefreshRequest(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientRefreshRequest request;
        jsonDecodeRefreshRequest(j, request);
        auto valid = request.validateBasic();
        if (valid.is_error()) return Result<ClientRefreshRequest>::fail(valid.errors());
        return Result<ClientRefreshRequest>::ok(std::move(request));
    } catch (const std::exception& e) {
        return Result<ClientRefreshRequest>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeRefreshResponse(const ClientRefreshResponse& response) const {
    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeRefreshResponse(response).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientRefreshResponse> ClientProtocolCodec::decodeRefreshResponse(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientRefreshResponse response;
        jsonDecodeRefreshResponse(j, response);
        auto valid = response.validateBasic();
        if (valid.is_error()) return Result<ClientRefreshResponse>::fail(valid.errors());
        return Result<ClientRefreshResponse>::ok(std::move(response));
    } catch (const std::exception& e) {
        return Result<ClientRefreshResponse>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeResetRequest(const ClientResetRequest& request) const {
    auto valid = request.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeResetRequest(request).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientResetRequest> ClientProtocolCodec::decodeResetRequest(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientResetRequest request;
        jsonDecodeResetRequest(j, request);
        auto valid = request.validateBasic();
        if (valid.is_error()) return Result<ClientResetRequest>::fail(valid.errors());
        return Result<ClientResetRequest>::ok(std::move(request));
    } catch (const std::exception& e) {
        return Result<ClientResetRequest>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<std::string> ClientProtocolCodec::encodeResetResponse(const ClientResetResponse& response) const {
    auto valid = response.validateBasic();
    if (valid.is_error()) return Result<std::string>::fail(valid.errors());
    try {
        return Result<std::string>::ok(jsonEncodeResetResponse(response).dump());
    } catch (const std::exception& e) {
        return Result<std::string>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

Result<ClientResetResponse> ClientProtocolCodec::decodeResetResponse(const std::string& json_str) const {
    try {
        auto j = json::parse(json_str);
        ClientResetResponse response;
        jsonDecodeResetResponse(j, response);
        auto valid = response.validateBasic();
        if (valid.is_error()) return Result<ClientResetResponse>::fail(valid.errors());
        return Result<ClientResetResponse>::ok(std::move(response));
    } catch (const std::exception& e) {
        return Result<ClientResetResponse>::fail(makeError(ErrorCode::command_invalid_format, e.what()));
    }
}

} // namespace pathfinder::client_protocol
