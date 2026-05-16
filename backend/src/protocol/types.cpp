#include "pathfinder/protocol/types.h"

namespace pathfinder::protocol {

int protocol_module_version() { return 1; }

// --- ProtocolMessageType ---

std::string toString(ProtocolMessageType type) {
    switch (type) {
        case ProtocolMessageType::Unknown: return "unknown";
        case ProtocolMessageType::Request: return "request";
        case ProtocolMessageType::Response: return "response";
        case ProtocolMessageType::Event: return "event";
        case ProtocolMessageType::Ack: return "ack";
        case ProtocolMessageType::Heartbeat: return "heartbeat";
        case ProtocolMessageType::Error: return "error";
        case ProtocolMessageType::Progress: return "progress";
    }
    return "unknown";
}

pathfinder::foundation::Result<ProtocolMessageType> protocolMessageTypeFromString(const std::string& str) {
    if (str == "request") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Request);
    if (str == "response") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Response);
    if (str == "event") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Event);
    if (str == "ack") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Ack);
    if (str == "heartbeat") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Heartbeat);
    if (str == "error") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Error);
    if (str == "progress") return pathfinder::foundation::Result<ProtocolMessageType>::ok(ProtocolMessageType::Progress);
    return pathfinder::foundation::Result<ProtocolMessageType>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "protocol.message_type_invalid",
            "Unknown ProtocolMessageType: " + str));
}

// --- ProtocolDomain ---

std::string toString(ProtocolDomain domain) {
    switch (domain) {
        case ProtocolDomain::Unknown: return "unknown";
        case ProtocolDomain::Command: return "command";
        case ProtocolDomain::Query: return "query";
        case ProtocolDomain::EventStream: return "event_stream";
        case ProtocolDomain::ProjectionSync: return "projection_sync";
        case ProtocolDomain::Save: return "save";
        case ProtocolDomain::Replay: return "replay";
        case ProtocolDomain::Config: return "config";
        case ProtocolDomain::Error: return "error";
        case ProtocolDomain::Health: return "health";
    }
    return "unknown";
}

pathfinder::foundation::Result<ProtocolDomain> protocolDomainFromString(const std::string& str) {
    if (str == "command") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Command);
    if (str == "query") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Query);
    if (str == "event_stream") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::EventStream);
    if (str == "projection_sync") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::ProjectionSync);
    if (str == "save") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Save);
    if (str == "replay") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Replay);
    if (str == "config") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Config);
    if (str == "error") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Error);
    if (str == "health") return pathfinder::foundation::Result<ProtocolDomain>::ok(ProtocolDomain::Health);
    return pathfinder::foundation::Result<ProtocolDomain>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "protocol.domain_invalid",
            "Unknown ProtocolDomain: " + str));
}

// --- ProtocolPayloadType ---

std::string toString(ProtocolPayloadType type) {
    switch (type) {
        case ProtocolPayloadType::Unknown: return "unknown";
        case ProtocolPayloadType::CommandResult: return "command_result";
        case ProtocolPayloadType::EventList: return "event_list";
        case ProtocolPayloadType::ReplayReport: return "replay_report";
        case ProtocolPayloadType::ErrorList: return "error_list";
        case ProtocolPayloadType::Text: return "text";
    }
    return "unknown";
}

pathfinder::foundation::Result<ProtocolPayloadType> protocolPayloadTypeFromString(const std::string& str) {
    if (str == "command_result") return pathfinder::foundation::Result<ProtocolPayloadType>::ok(ProtocolPayloadType::CommandResult);
    if (str == "event_list") return pathfinder::foundation::Result<ProtocolPayloadType>::ok(ProtocolPayloadType::EventList);
    if (str == "replay_report") return pathfinder::foundation::Result<ProtocolPayloadType>::ok(ProtocolPayloadType::ReplayReport);
    if (str == "error_list") return pathfinder::foundation::Result<ProtocolPayloadType>::ok(ProtocolPayloadType::ErrorList);
    if (str == "text") return pathfinder::foundation::Result<ProtocolPayloadType>::ok(ProtocolPayloadType::Text);
    return pathfinder::foundation::Result<ProtocolPayloadType>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::validation_enum_unknown,
            "protocol.payload_type_invalid",
            "Unknown ProtocolPayloadType: " + str));
}

} // namespace pathfinder::protocol
