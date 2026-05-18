#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/learning/learning_loop.h"
#include <string>

namespace pathfinder::h5_dialog {

class DialogLearningAdapter {
public:
    pathfinder::foundation::Result<pathfinder::learning::LearningLoopInput> buildLearningInput(
        const DialogSessionState& state,
        const DialogIntent& intent,
        const DialogFeedbackTemplate& feedback) const;

    pathfinder::foundation::Result<void> applyLearningResult(
        DialogSessionState& state,
        const pathfinder::learning::LearningLoopResult& result) const;
};

} // namespace pathfinder::h5_dialog
