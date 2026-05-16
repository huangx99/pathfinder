#include "pathfinder/pipeline/context.h"

namespace pathfinder::pipeline {

pathfinder::foundation::Result<void> PipelineContext::validateBasic() const {
    // Validate command
    auto cmd_result = command.validateBasic();
    if (cmd_result.is_error()) {
        return cmd_result;
    }

    // Validate state_id is not empty
    if (state_metadata.state_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "PipelineContext state_id is empty"
            )
        );
    }

    // Validate state_id format
    if (!pathfinder::foundation::isValidIdString(state_metadata.state_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "PipelineContext state_id has invalid format"
            )
        );
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::pipeline
