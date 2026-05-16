#include "pathfinder/state/state_change.h"
#include <set>

namespace pathfinder::state {

pathfinder::foundation::Result<void> StateChange::validateBasic() const {
    // Check change_id is not empty
    if (change_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "StateChange change_id is empty"
            )
        );
    }

    // Check change_id format
    if (!pathfinder::foundation::isValidIdString(change_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "StateChange change_id has invalid format"
            )
        );
    }

    // Check target_id format (if not empty)
    if (!target_id.empty() && !pathfinder::foundation::isValidIdString(target_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "StateChange target_id has invalid format"
            )
        );
    }

    // NoOp must have reason
    if (operation == StateChangeOperation::NoOp && reason.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "StateChange NoOp must have reason"
            )
        );
    }

    // Update/Delete/Append/Remove must have non-empty field_path
    if (operation != StateChangeOperation::Create &&
        operation != StateChangeOperation::NoOp &&
        field_path.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::validation_failed,
                "StateChange field_path is empty for operation"
            )
        );
    }

    // Create/Update/Append must have after_value with non-Empty type
    if (operation == StateChangeOperation::Create ||
        operation == StateChangeOperation::Update ||
        operation == StateChangeOperation::Append) {
        if (!after_value.has_value() || after_value->type == StateValueType::Empty) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::validation_failed,
                    "StateChange after_value is required for Create/Update/Append"
                )
            );
        }
    }

    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> StateChangeSet::addChange(StateChange change) {
    // Check for duplicate change_id
    for (const auto& existing : changes_) {
        if (existing.change_id == change.change_id) {
            return pathfinder::foundation::Result<void>::fail(
                pathfinder::foundation::makeError(
                    pathfinder::foundation::ErrorCode::command_duplicate,
                    "Duplicate change_id"
                )
            );
        }
    }

    // Validate the change
    auto result = change.validateBasic();
    if (result.is_error()) {
        return result;
    }

    changes_.push_back(std::move(change));
    return pathfinder::foundation::Result<void>::ok();
}

pathfinder::foundation::Result<void> StateChangeSet::validateBasic() const {
    for (const auto& change : changes_) {
        auto result = change.validateBasic();
        if (result.is_error()) {
            return result;
        }
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::state
