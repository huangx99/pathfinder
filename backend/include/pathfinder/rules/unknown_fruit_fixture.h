#pragma once

#include "pathfinder/state/game_state.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::rules {

// UnknownFruitFixture provides a stable, reproducible test scenario
// for the P3 unknown fruit + eat closed loop.
class UnknownFruitFixture {
public:
    // Create the initial game state for the unknown fruit scenario.
    // Contains:
    // - actor_001 with hunger=80, health=100
    // - unknown_fruit definition with edible_profile (hunger_delta=-20, health_delta=0, Edible)
    // - obj_unknown_fruit_001 pointing to unknown_fruit definition
    // - Empty cognition state (no claims)
    static pathfinder::state::GameState createInitialState();

    // Create a CommandEnvelope for eating the unknown fruit.
    // Contains:
    // - command_id: cmd_try_eat_001
    // - action_id: eat
    // - actor_id: actor_001
    // - target: obj_unknown_fruit_001
    // - intent: experiment
    static pathfinder::command::CommandEnvelope createEatCommand();
};

} // namespace pathfinder::rules
