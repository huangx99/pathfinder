#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/client_protocol/client_protocol_codec.h"
#include "pathfinder/client_protocol/client_session_gateway.h"
#include "pathfinder/client_protocol/client_command_gateway.h"
#include "json.hpp"
#include <sstream>

namespace pathfinder::client_http {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

ClientHttpGateway::ClientHttpGateway(
    pathfinder::client_protocol::ClientProtocolCodec& codec,
    pathfinder::client_protocol::ClientSessionGateway& session_gateway,
    pathfinder::client_protocol::ClientCommandGateway& command_gateway)
    : codec_(codec)
    , session_gateway_(session_gateway)
    , command_gateway_(command_gateway) {
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
