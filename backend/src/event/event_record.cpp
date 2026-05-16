#include "pathfinder/event/event_record.h"

namespace pathfinder::event {

pathfinder::foundation::Result<void> EventRecord::validateBasic() const {
    // Check event_id
    if (event_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "EventRecord event_id is empty"
            )
        );
    }

    // Check event_id format
    if (!pathfinder::foundation::isValidIdString(event_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "EventRecord event_id has invalid format"
            )
        );
    }

    // Check command_id format (if present)
    if (command_id.has_value() && !command_id->empty() && !pathfinder::foundation::isValidIdString(command_id->value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "EventRecord command_id has invalid format"
            )
        );
    }

    // Check state_change_ids format
    for (const auto& sc_id : state_change_ids) {
        if (!sc_id.empty() && !pathfinder::foundation::isValidIdString(sc_id.value())) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::id_invalid_format,
                    "EventRecord state_change_id has invalid format"
                )
            );
        }
    }

    // Check target_ids format
    for (const auto& t_id : target_ids) {
        if (!t_id.empty() && !pathfinder::foundation::isValidIdString(t_id.value())) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::id_invalid_format,
                    "EventRecord target_id has invalid format"
                )
            );
        }
    }

    // Check event_type is not Unknown
    if (event_type == EventType::Unknown) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "EventRecord event_type is Unknown"
            )
        );
    }

    // Check payload_type is not empty
    if (payload.payload_type.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "EventRecord payload_type is empty"
            )
        );
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::event
