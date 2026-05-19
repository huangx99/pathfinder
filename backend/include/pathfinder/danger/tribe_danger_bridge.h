#pragma once

#include "pathfinder/danger/danger_types.h"

namespace pathfinder::danger {

class TribeDangerBridge {
public:
    pathfinder::foundation::Result<pathfinder::tribe::TribeStateInput> toTribeStateInput(
        const std::vector<TribeDangerPressureDraft>& drafts,
        pathfinder::foundation::TribeId tribe_id,
        pathfinder::foundation::Tick tick) const;
};

} // namespace pathfinder::danger
