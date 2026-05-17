#pragma once

#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::protocol {

int protocol_module_version();

// Protocol message types
enum class ProtocolMessageType {
    Unknown,
    Request,
    Response,
    Event,
    Ack,
    Heartbeat,
    Error,
    Progress
};

std::string toString(ProtocolMessageType type);
pathfinder::foundation::Result<ProtocolMessageType> protocolMessageTypeFromString(const std::string& str);

// Protocol domains
enum class ProtocolDomain {
    Unknown,
    Command,
    Query,
    EventStream,
    ProjectionSync,
    Save,
    Replay,
    Config,
    Error,
    Health
};

std::string toString(ProtocolDomain domain);
pathfinder::foundation::Result<ProtocolDomain> protocolDomainFromString(const std::string& str);

// Protocol payload types
enum class ProtocolPayloadType {
    Unknown,
    CommandResult,
    EventList,
    ReplayReport,
    ErrorList,
    Text,
    AgentHistoryProjection,
    AgentReplayLockProjection,
    AgentTrainingSampleProjection
};

std::string toString(ProtocolPayloadType type);
pathfinder::foundation::Result<ProtocolPayloadType> protocolPayloadTypeFromString(const std::string& str);

} // namespace pathfinder::protocol
