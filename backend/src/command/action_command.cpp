#include "pathfinder/command/action_command.h"

namespace pathfinder::command {

pathfinder::foundation::Result<void> ActionCommand::validateBasic() const {
    // Check action_id is not empty and format valid
    if (action_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "ActionCommand: action_id is empty"
            )
        );
    }
    if (!pathfinder::foundation::isValidIdString(action_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "ActionCommand: action_id has invalid format: " + action_id.value()
            )
        );
    }

    // Check actor_id is not empty and format valid
    if (actor_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "ActionCommand: actor_id is empty"
            )
        );
    }
    if (!pathfinder::foundation::isValidIdString(actor_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "ActionCommand: actor_id has invalid format: " + actor_id.value()
            )
        );
    }

    // Check targets is not empty
    if (targets.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::command_missing_required_field,
                "ActionCommand: targets is empty"
            )
        );
    }

    // Validate each target
    for (size_t i = 0; i < targets.size(); ++i) {
        auto target_result = targets[i].validateBasic();
        if (target_result.is_error()) {
            // Propagate the first target error
            return target_result;
        }
    }

    // intent is optional, Unknown is allowed
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::command
