#pragma once

#include "pathfinder/h5_playable/playable_types.h"
#include <string>

namespace pathfinder::h5_playable {

class H5PlayableWireCodec {
public:
    pathfinder::foundation::Result<H5PlayableBootstrapRequest> parseBootstrapForm(const std::string& body) const;
    pathfinder::foundation::Result<H5PlayableRequest> parseTurnForm(const std::string& body) const;
    pathfinder::foundation::Result<std::string> encodeResponse(const H5PlayableResponse& response) const;
};

std::string playableJsonEscape(const std::string& value);

} // namespace pathfinder::h5_playable
