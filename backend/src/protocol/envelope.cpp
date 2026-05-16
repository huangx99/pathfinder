#include "pathfinder/protocol/envelope.h"
#include "pathfinder/foundation/id.h"

namespace pathfinder::protocol {

pathfinder::foundation::Result<void> ProtocolEnvelope::validateBasic() const {
    // protocol_version must be non-empty
    if (protocol_version.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "protocol.protocol_version_missing"));
    }

    // message_id must be non-empty and valid format
    if (message_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "protocol.message_id_missing"));
    }
    if (!pathfinder::foundation::isValidIdString(message_id)) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "protocol.message_id_invalid"));
    }

    // message_type must not be Unknown
    if (message_type == ProtocolMessageType::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "protocol.message_type_unknown"));
    }

    // domain must not be Unknown
    if (domain == ProtocolDomain::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "protocol.domain_unknown"));
    }

    // payload_type must not be Unknown
    // Exception: Error-type envelope allows ErrorList
    if (payload.payload_type == ProtocolPayloadType::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_enum_unknown,
                "protocol.payload_type_unknown"));
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::protocol
