#include "pathfinder/command/target.h"

namespace pathfinder::command {

pathfinder::foundation::Result<void> ActionTarget::validateBasic() const {
    // Check target_type is not None
    if (target_type == ActionTargetType::None) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::command_missing_required_field,
                "ActionTarget: target_type is none"
            )
        );
    }

    // Check target_id is not empty
    if (target_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "ActionTarget: target_id is empty"
            )
        );
    }

    // Check target_id format is valid
    if (!pathfinder::foundation::isValidIdString(target_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "ActionTarget: target_id has invalid format: " + target_id.value()
            )
        );
    }

    // Role is always valid as an enum value
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::command
