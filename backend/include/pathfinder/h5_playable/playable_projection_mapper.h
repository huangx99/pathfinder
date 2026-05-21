// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/h5_projection/projection.h"

namespace pathfinder::h5_playable {

class H5PlayableProjectionMapper {
public:
    pathfinder::foundation::Result<pathfinder::h5_projection::H5ProjectionSourceBundle> buildSourceBundle(
        const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
        const pathfinder::h5_dialog::DialogSessionState& session_state) const;
};

std::string playableSafeReplyText(const pathfinder::h5_dialog::DialogResponseDto& dialog_response);

} // namespace pathfinder::h5_playable
