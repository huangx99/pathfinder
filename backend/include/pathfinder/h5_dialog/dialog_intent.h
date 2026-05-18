#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include <string>

namespace pathfinder::h5_dialog {

class DialogIntentParser {
public:
    pathfinder::foundation::Result<DialogIntent> parse(const std::string& input_text) const;
};

} // namespace pathfinder::h5_dialog
