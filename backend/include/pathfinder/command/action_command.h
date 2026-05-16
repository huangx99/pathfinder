#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/types.h"
#include "pathfinder/command/target.h"
#include <vector>

namespace pathfinder::command {

// ActionCommand represents a single action intent
// P1 only: carries data and does basic validation
// Does NOT execute rules or modify state
struct ActionCommand {
    pathfinder::foundation::ActionId action_id;
    pathfinder::foundation::EntityId actor_id;
    std::vector<ActionTarget> targets;
    CommandIntent intent = CommandIntent::Unknown;

    // P1 basic validation
    // Checks: action_id valid, actor_id valid, targets non-empty, each target valid
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::command
