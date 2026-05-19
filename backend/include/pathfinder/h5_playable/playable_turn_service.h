#pragma once

#include "pathfinder/h5_dialog/dialog_turn_service.h"
#include "pathfinder/h5_playable/playable_projection_mapper.h"
#include "pathfinder/h5_playable/playable_types.h"

namespace pathfinder::h5_playable {

class H5PlayableTurnService {
public:
    pathfinder::foundation::Result<H5PlayableResponse> bootstrap(const H5PlayableBootstrapRequest& request);
    pathfinder::foundation::Result<H5PlayableResponse> handleTurn(const H5PlayableRequest& request);

private:
    pathfinder::h5_dialog::DialogTurnService dialog_service_;
    H5PlayableProjectionMapper mapper_;
    pathfinder::h5_projection::H5ProjectionComposer composer_;

    pathfinder::foundation::Result<H5PlayableResponse> buildResponse(
        const pathfinder::h5_dialog::DialogResponseDto& dialog_response,
        const pathfinder::h5_dialog::DialogSessionState& session_state,
        PlayableFeedbackTone tone,
        const std::string& request_key) const;
};

} // namespace pathfinder::h5_playable
