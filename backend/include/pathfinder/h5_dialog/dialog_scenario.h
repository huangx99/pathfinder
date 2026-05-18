#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include <string>
#include <vector>
#include <optional>

namespace pathfinder::h5_dialog {

class DialogScenarioCatalog {
public:
    pathfinder::foundation::Result<DialogScenario> defaultScenario() const;

    pathfinder::foundation::Result<const DialogScenarioObject*> findObject(
        const DialogScenario& scenario,
        const std::string& object_key) const;

    pathfinder::foundation::Result<DialogFeedbackTemplate> findFeedback(
        const DialogScenario& scenario,
        const std::string& object_key,
        DialogActionKind action) const;
};

} // namespace pathfinder::h5_dialog
