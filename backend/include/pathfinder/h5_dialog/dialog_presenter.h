// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/learning/learning_loop.h"
#include <string>

namespace pathfinder::h5_dialog {

class DialogPresenter {
public:
    pathfinder::foundation::Result<DialogResponseDto> buildObserveResponse(
        const DialogSessionState& state) const;

    pathfinder::foundation::Result<DialogResponseDto> buildLearningResponse(
        const DialogSessionState& state,
        const DialogIntent& intent,
        const pathfinder::learning::LearningLoopResult& result,
        const pathfinder::learning::LearningDebugReport& report,
        const std::string& observed_effect_key = "") const;

    pathfinder::foundation::Result<DialogResponseDto> buildInspectResponse(
        const DialogSessionState& state,
        bool inspect_recipient) const;

    pathfinder::foundation::Result<DialogResponseDto> buildRejectedResponse(
        const DialogSessionState& state,
        const DialogIntent& intent,
        const std::string& reason_key) const;

    pathfinder::foundation::Result<DialogResponseDto> buildHelpResponse(
        const DialogSessionState& state) const;

    pathfinder::foundation::Result<DialogResponseDto> buildRestartResponse(
        const DialogSessionState& state) const;
};

} // namespace pathfinder::h5_dialog
