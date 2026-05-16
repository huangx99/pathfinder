#pragma once

#include "pathfinder/state/game_state.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/state/state_change.h"
#include "pathfinder/event/event_record.h"
#include "pathfinder/cognition/types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::rules {

// Input for the EatObjectResolver
struct EatObjectResolveInput {
    const pathfinder::command::CommandEnvelope& command;
    const pathfinder::state::GameState& state;
};

// Draft output from the EatObjectResolver
struct EatObjectResolveDraft {
    pathfinder::state::StateChangeSet state_changes;
    std::vector<pathfinder::event::EventRecord> events;
    std::vector<pathfinder::cognition::CognitionEvidence> cognition_evidence;
};

// Resolver for the "eat" action on objects
class EatObjectResolver {
public:
    // Check if this resolver can handle the given command
    static bool canResolve(const pathfinder::command::CommandEnvelope& command);

    // Resolve the eat command into state changes, events, and cognition evidence.
    // Does NOT modify the GameState directly.
    static pathfinder::foundation::Result<EatObjectResolveDraft> resolve(const EatObjectResolveInput& input);
};

} // namespace pathfinder::rules
