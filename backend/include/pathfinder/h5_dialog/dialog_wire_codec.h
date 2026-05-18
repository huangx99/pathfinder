#pragma once

#include "pathfinder/h5_dialog/dialog_types.h"
#include <string>

namespace pathfinder::h5_dialog {

class DialogWireCodec {
public:
    pathfinder::foundation::Result<DialogRequestDto> parseFormUrlEncodedRequest(
        const std::string& body) const;

    pathfinder::foundation::Result<std::string> encodeJsonResponse(
        const DialogResponseDto& response) const;
};

} // namespace pathfinder::h5_dialog
