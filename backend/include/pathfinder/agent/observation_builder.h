#pragma once

#include "pathfinder/agent/builder_types.h"

namespace pathfinder::agent {

// ObservationBuilder: builds AgentObservation from GameState + VisibilityInput
// Reads GameState but does NOT modify it
// Does NOT execute game logic or resolve rules
// Does NOT expose real effect profiles to agents
class ObservationBuilder {
public:
    foundation::Result<ObservationBuildResult> build(const ObservationBuildRequest& request) const;
};

} // namespace pathfinder::agent
