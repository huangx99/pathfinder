#include "pathfinder/command/envelope.h"

namespace pathfinder::command {

pathfinder::foundation::Result<void> CommandEnvelope::validateBasic() const {
    // Check command_id is not empty and format valid
    if (command_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "CommandEnvelope: command_id is empty"
            )
        );
    }
    if (!pathfinder::foundation::isValidIdString(command_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "CommandEnvelope: command_id has invalid format: " + command_id.value()
            )
        );
    }

    // Check source is not Unknown (invalid source)
    if (source == CommandSource::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::command_invalid_format,
                "CommandEnvelope: source is unknown/invalid"
            )
        );
    }

    // Check idempotency_key length if present
    if (idempotency_key.has_value() && idempotency_key->length() > kMaxKeyLength) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::command_invalid_argument,
                "CommandEnvelope: idempotency_key too long (max " + std::to_string(kMaxKeyLength) + ")"
            )
        );
    }

    // Check correlation_id length if present
    if (correlation_id.has_value() && correlation_id->length() > kMaxKeyLength) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::command_invalid_argument,
                "CommandEnvelope: correlation_id too long (max " + std::to_string(kMaxKeyLength) + ")"
            )
        );
    }

    // Validate payload
    auto payload_result = payload.validateBasic();
    if (payload_result.is_error()) {
        return payload_result;
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::command
