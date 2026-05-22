#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/world_map_interaction/world_map_interaction_types.h"
#include "json.hpp"
#include <sstream>
#include <utility>

namespace pathfinder::client_http {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

ClientHttpGateway::ClientHttpGateway(
    pathfinder::client_protocol::ClientProtocolCodec& codec,
    pathfinder::client_protocol::ClientSessionGateway& session_gateway,
    pathfinder::client_protocol::ClientCommandGateway& command_gateway,
    pathfinder::world_map_interaction::ClientMapSelectionService* selection_service,
    ResetWorldCallback reset_world_callback)
    : codec_(codec)
    , session_gateway_(session_gateway)
    , command_gateway_(command_gateway)
    , selection_service_(selection_service)
    , reset_world_callback_(std::move(reset_world_callback)) {
}

void ClientHttpGateway::setMapSelectionService(
    pathfinder::world_map_interaction::ClientMapSelectionService* service) {
    selection_service_ = service;
}

ClientHttpResponse ClientHttpGateway::handleApi(const ClientHttpRequest& request) {
    if (request.method != ClientHttpMethod::Post) {
        return makeJsonError(405, "method_not_allowed", "Only POST is allowed for API endpoints");
    }

    if (request.path == "/api/client/bootstrap") {
        return handleBootstrap(request.body);
    }
    if (request.path == "/api/client/command") {
        return handleCommand(request.body);
    }
    if (request.path == "/api/client/refresh") {
        return handleRefresh(request.body);
    }
    if (request.path == "/api/client/reset") {
        return handleReset(request.body);
    }
    if (request.path == "/api/client/select") {
        return handleSelect(request.body);
    }

    return makeJsonError(404, "not_found", "API endpoint not found: " + request.path);
}

ClientHttpResponse ClientHttpGateway::handleBootstrap(const std::string& body) {
    auto decoded = codec_.decodeBootstrapRequest(body);
    if (decoded.is_error()) {
        return makeJsonError(400, "bad_request", "decode bootstrap request failed",
            {"validation_failed"});
    }

    auto response = session_gateway_.bootstrap(decoded.value());
    if (response.is_error()) {
        return makeJsonError(500, "bootstrap_failed", "bootstrap processing failed");
    }

    auto encoded = codec_.encodeBootstrapResponse(response.value());
    if (encoded.is_error()) {
        return makeJsonError(500, "encode_failed", "encode bootstrap response failed");
    }

    return makeJsonResponse(200, encoded.value());
}

ClientHttpResponse ClientHttpGateway::handleCommand(const std::string& body) {
    auto decoded = codec_.decodeCommandRequest(body);
    if (decoded.is_error()) {
        return makeJsonError(400, "bad_request", "decode command request failed",
            {"validation_failed"});
    }

    auto response = command_gateway_.handleCommand(decoded.value());
    if (response.is_error()) {
        return makeJsonError(500, "command_failed", "command processing failed");
    }

    auto encoded = codec_.encodeCommandResponse(response.value());
    if (encoded.is_error()) {
        return makeJsonError(500, "encode_failed", "encode command response failed");
    }

    return makeJsonResponse(200, encoded.value());
}

ClientHttpResponse ClientHttpGateway::handleRefresh(const std::string& body) {
    auto decoded = codec_.decodeRefreshRequest(body);
    if (decoded.is_error()) {
        return makeJsonError(400, "bad_request", "decode refresh request failed",
            {"validation_failed"});
    }

    auto response = session_gateway_.refresh(decoded.value());
    if (response.is_error()) {
        return makeJsonError(500, "refresh_failed", "refresh processing failed");
    }

    auto encoded = codec_.encodeRefreshResponse(response.value());
    if (encoded.is_error()) {
        return makeJsonError(500, "encode_failed", "encode refresh response failed");
    }

    return makeJsonResponse(200, encoded.value());
}

ClientHttpResponse ClientHttpGateway::handleReset(const std::string& body) {
    auto decoded = codec_.decodeResetRequest(body);
    if (decoded.is_error()) {
        return makeJsonError(400, "bad_request", "decode reset request failed",
            {"validation_failed"});
    }

    if (reset_world_callback_) {
        auto reset_world_res = reset_world_callback_();
        if (reset_world_res.is_error()) {
            return makeJsonError(500, "world_reset_failed", "world reset processing failed");
        }
    }

    auto response = session_gateway_.reset(decoded.value());
    if (response.is_error()) {
        return makeJsonError(500, "reset_failed", "reset processing failed");
    }

    auto encoded = codec_.encodeResetResponse(response.value());
    if (encoded.is_error()) {
        return makeJsonError(500, "encode_failed", "encode reset response failed");
    }

    return makeJsonResponse(200, encoded.value());
}

ClientHttpResponse ClientHttpGateway::handleSelect(const std::string& body) {
    using namespace pathfinder::world_map_interaction;

    if (!selection_service_) {
        return makeJsonError(503, "service_unavailable", "Map selection service not configured");
    }

    // Manual JSON decode for select request
    ClientMapSelectionRequest request;
    try {
        auto j = nlohmann::json::parse(body);
        request.session_id = j.value("session_id", "");
        request.client_id = j.value("client_id", "");
        request.known_projection_version = j.value("known_projection_version", 0);
        request.layer_key = j.value("layer_key", "surface");
        request.cell_coord.x = j.value("x", 0);
        request.cell_coord.y = j.value("y", 0);
        request.cell_coord.layer_key = request.layer_key;
        request.entity_id = j.value("entity_id", "");
        request.resource_node_id = j.value("resource_node_id", "");
        request.inventory_item_id = j.value("inventory_item_id", "");

        std::string kind_str = j.value("selection_kind", "Cell");
        auto kind_res = mapSelectionKindFromString(kind_str);
        if (kind_res.is_ok()) {
            request.selection_kind = kind_res.value();
        }
    } catch (const std::exception& e) {
        return makeJsonError(400, "bad_request", std::string("JSON parse failed: ") + e.what());
    }

    // Get actor key from session
    auto actor_res = session_gateway_.getSessionActorKey(request.session_id);
    if (actor_res.is_error()) {
        return makeJsonError(401, "session_invalid", "Session not found or invalid");
    }
    std::string actor_key = actor_res.value();

    // Perform selection
    auto select_res = selection_service_->select(request, actor_key);
    if (select_res.is_error()) {
        std::string err_msg = "select_failed";
        if (!select_res.errors().empty()) {
            err_msg = select_res.errors()[0].message_key;
        }
        return makeJsonError(500, "select_failed", err_msg);
    }

    const auto& response = select_res.value();

    // Update session available commands with selection-specific options only when selection is current.
    if (!response.stale && !response.requires_full_refresh && !response.available_options.empty()) {
        auto update_res = session_gateway_.updateAvailableCommands(
            request.session_id, response.available_options);
        if (update_res.is_error()) {
            // Non-fatal: log but continue
        }
    }

    // Manual JSON encode for select response
    nlohmann::json j_resp;
    j_resp["session_id"] = response.session_id;
    j_resp["selection_id"] = response.selection_id;
    j_resp["valid"] = response.valid;
    j_resp["stale"] = response.stale;
    j_resp["requires_full_refresh"] = response.requires_full_refresh;
    j_resp["selected_cell_x"] = response.selected_cell.x;
    j_resp["selected_cell_y"] = response.selected_cell.y;
    j_resp["selected_cell_layer"] = response.selected_cell.layer_key;
    j_resp["selected_cell_terrain"] = response.selected_cell_terrain_key;

    j_resp["selected_objects"] = nlohmann::json::array();
    for (const auto& obj : response.selected_objects) {
        nlohmann::json j_obj;
        j_obj["object_id"] = obj.object_id;
        j_obj["object_kind"] = obj.object_kind;
        j_obj["display_name"] = obj.display_name_zh;
        j_obj["tags"] = obj.tag_keys;
        j_resp["selected_objects"].push_back(j_obj);
    }

    j_resp["allowed_target_kinds"] = response.allowed_target_kinds;

    j_resp["available_options"] = nlohmann::json::array();
    for (const auto& opt : response.available_options) {
        nlohmann::json j_opt;
        j_opt["option_id"] = opt.option_id;
        j_opt["command_key"] = opt.command_key;
        j_opt["label"] = opt.label_text;
        j_opt["enabled"] = opt.enabled;
        if (!opt.disabled_reason_text.empty()) {
            j_opt["disabled_reason"] = opt.disabled_reason_text;
        }
        j_opt["priority"] = opt.priority;
        j_resp["available_options"].push_back(j_opt);
    }

    j_resp["warning_keys"] = response.warning_keys;

    return makeJsonResponse(200, j_resp.dump());
}

ClientHttpResponse ClientHttpGateway::makeJsonError(
    int status_code,
    const std::string& error_key,
    const std::string& message) {
    return makeJsonError(status_code, error_key, message, {});
}

ClientHttpResponse ClientHttpGateway::makeJsonError(
    int status_code,
    const std::string& error_key,
    const std::string& message,
    const std::vector<std::string>& details) {
    nlohmann::json j;
    j["ok"] = false;
    j["error_key"] = error_key;
    j["message"] = message;
    j["details"] = details;
    return ClientHttpResponse{
        status_code,
        "application/json; charset=utf-8",
        {},
        j.dump()
    };
}

ClientHttpResponse ClientHttpGateway::makeJsonResponse(int status_code, const std::string& json_body) {
    return ClientHttpResponse{
        status_code,
        "application/json; charset=utf-8",
        {},
        json_body
    };
}

} // namespace pathfinder::client_http
