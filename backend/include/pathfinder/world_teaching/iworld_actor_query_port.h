#pragma once

#include "pathfinder/world_runtime/world_runtime_types.h"
#include <optional>
#include <string>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// IWorldActorQueryPort
// ---------------------------------------------------------------------------
// Read-only port for querying actor runtime state.
// Used by teaching eligibility to verify distance constraints.
// ---------------------------------------------------------------------------

class IWorldActorQueryPort {
public:
    virtual ~IWorldActorQueryPort() = default;

    virtual std::optional<world_runtime::WorldActorRuntime> findActor(
        const std::string& actor_key) const = 0;
};

} // namespace pathfinder::world_teaching
