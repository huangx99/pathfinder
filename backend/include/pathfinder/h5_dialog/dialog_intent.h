// DEPRECATED: Legacy H5 prototype code. Do not extend for new gameplay.
// New clients must use the generic Client Protocol (P53+) and WorldCommand pipeline.

#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include <string>

namespace pathfinder::h5_dialog {

class DialogIntentParser {
public:
    pathfinder::foundation::Result<DialogIntent> parse(const std::string& input_text) const;
    pathfinder::foundation::Result<DialogIntent> parseWithScenario(const std::string& input_text, const DialogScenario& scenario) const;
};

} // namespace pathfinder::h5_dialog
