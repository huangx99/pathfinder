#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/types.h"

namespace pathfinder::command {

// ActionTarget represents a single target reference in a command
// P1 only: carries data and does basic validation
// Does NOT check if the target actually exists
struct ActionTarget {
    ActionTargetType target_type = ActionTargetType::None;
    pathfinder::foundation::TargetId target_id;
    ActionTargetRole role = ActionTargetRole::Primary;

    // P1 basic validation
    // Checks: target_type not none, target_id format valid, role valid
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::command
