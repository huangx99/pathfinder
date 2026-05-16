#pragma once

#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/result.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/config/package.h"
#include <optional>
#include <string>

namespace pathfinder::pipeline {

// Pipeline context
struct PipelineContext {
    pathfinder::command::CommandEnvelope command;
    pathfinder::state::GameStateMetadata state_metadata;
    // P3: full game state for rule execution
    pathfinder::state::GameState* game_state = nullptr;
    std::optional<pathfinder::foundation::ConfigVersionId> config_version;
    pathfinder::foundation::RandomSeed random_seed;
    std::optional<std::string> correlation_id;

    // Validate basic structure
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::pipeline
