#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/protocol/types.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::protocol {

// Protocol payload
struct ProtocolPayload {
    ProtocolPayloadType payload_type = ProtocolPayloadType::Unknown;
    std::string message_key;
    std::string debug_text;
};

// Protocol error (summary, not internal detail)
struct ProtocolError {
    std::string code;
    std::string message_key;
    std::string debug_text;
};

// Protocol warning
struct ProtocolWarning {
    std::string code;
    std::string message_key;
    std::string debug_text;
};

// Protocol envelope - unified message wrapper
struct ProtocolEnvelope {
    std::string protocol_version;
    std::string message_id;
    ProtocolMessageType message_type = ProtocolMessageType::Unknown;
    ProtocolDomain domain = ProtocolDomain::Unknown;
    std::optional<std::string> request_id;
    std::optional<std::string> correlation_id;
    std::optional<std::string> session_id;
    std::optional<int64_t> client_seq;
    std::optional<int64_t> server_seq;
    std::optional<pathfinder::foundation::StateVersion> state_version;
    std::optional<pathfinder::foundation::ConfigVersionId> config_version_id;
    ProtocolPayload payload;
    std::vector<ProtocolError> errors;
    std::vector<ProtocolWarning> warnings;

    // Validate basic structure
    // Checks: protocol_version non-empty, message_id non-empty and valid format,
    // message_type/domain/payload_type non-Unknown
    // Error-type envelope allows payload_type ErrorList
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::protocol
