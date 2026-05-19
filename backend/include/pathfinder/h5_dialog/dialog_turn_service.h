#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include "pathfinder/h5_dialog/dialog_intent.h"
#include "pathfinder/h5_dialog/dialog_scenario.h"
#include "pathfinder/h5_dialog/dialog_session.h"
#include "pathfinder/h5_dialog/dialog_learning_adapter.h"
#include "pathfinder/h5_dialog/dialog_presenter.h"
#include "pathfinder/learning/learning_loop.h"
#include <memory>

namespace pathfinder::h5_dialog {

struct DialogTurnServiceResult {
    DialogResponseDto response;
    DialogSessionState state;
};

class DialogTurnService {
public:
    DialogTurnService();

    pathfinder::foundation::Result<DialogResponseDto> handle(
        const DialogRequestDto& request);

    pathfinder::foundation::Result<DialogTurnServiceResult> handleDetailed(
        const DialogRequestDto& request);

    pathfinder::foundation::Result<DialogSessionState> loadOrCreateSessionSnapshot(
        const std::string& session_id);

    pathfinder::foundation::Result<void> saveSessionSnapshot(
        const DialogSessionState& state);

    pathfinder::foundation::Result<void> resetSession(
        const std::string& session_id);

private:
    DialogIntentParser intent_parser_;
    DialogScenarioCatalog scenario_catalog_;
    std::unique_ptr<DialogSessionStore> session_store_;
    DialogLearningAdapter learning_adapter_;
    DialogPresenter presenter_;
    pathfinder::learning::LearningLoopService learning_service_;
    pathfinder::learning::LearningDebugReportBuilder report_builder_;
};

} // namespace pathfinder::h5_dialog
