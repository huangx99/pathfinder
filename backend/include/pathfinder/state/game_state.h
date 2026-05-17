#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/hash.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/state/types.h"
#include "pathfinder/state/actor_state.h"
#include "pathfinder/object/world_object.h"
#include "pathfinder/cognition/cognition_state.h"
#include "pathfinder/cognition/cognition_v2_state.h"

namespace pathfinder::state {

// GameState metadata
struct GameStateMetadata {
    pathfinder::foundation::GameStateId state_id;
    pathfinder::foundation::StateVersion state_version;
    pathfinder::foundation::Tick current_tick;
    pathfinder::foundation::ConfigVersionId config_version;
    pathfinder::foundation::HashValue state_hash;
};

// GameState - minimal snapshot
struct GameState {
    GameStateMetadata metadata;

    // P3: object, actor, cognition stores
    pathfinder::object::ObjectStore object_store;
    ActorStateStore actor_store;
    pathfinder::cognition::CognitionState cognition_state;

    // P15: formal cognition layer
    pathfinder::cognition::CognitionStateV2 cognition_state_v2;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;

    // Clone metadata only (for testing pipeline doesn't modify input)
    GameState cloneMetadataOnly() const;
};

} // namespace pathfinder::state
