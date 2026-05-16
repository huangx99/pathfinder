#pragma once

#include "pathfinder/state/game_state.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::state {

class MinimalStateApplier {
public:
    // Apply validated state changes to the game state.
    // Must be called AFTER StateChangeGate::validate.
    static pathfinder::foundation::Result<void> apply(GameState& state, const StateChangeSet& changes);
};

} // namespace pathfinder::state
