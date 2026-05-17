#include "pathfinder/state/game_state.h"

namespace pathfinder::state {

pathfinder::foundation::Result<void> GameState::validateBasic() const {
    // Check state_id is not empty
    if (metadata.state_id.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_missing,
                "GameState state_id is empty"
            )
        );
    }

    // Check state_id format
    if (!pathfinder::foundation::isValidIdString(metadata.state_id.value())) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(
                pathfinder::foundation::ErrorCode::id_invalid_format,
                "GameState state_id has invalid format"
            )
        );
    }

    // StateVersion and Tick are uint64_t, always >= 0
    // No need to check for negative values

    // Validate P15 cognition state
    auto cog_v2_result = cognition_state_v2.validateBasic();
    if (cog_v2_result.is_error()) {
        return cog_v2_result;
    }

    return pathfinder::foundation::Result<void>::ok();
}

GameState GameState::cloneMetadataOnly() const {
    GameState copy;
    copy.metadata = metadata;
    return copy;
}

} // namespace pathfinder::state
