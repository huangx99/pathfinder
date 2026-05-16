#pragma once

#include "pathfinder/agent/builder_types.h"

namespace pathfinder::agent {

// ActionSpaceBuilder: builds AgentActionSpace from AgentObservation
// Reads AgentObservation only, does NOT access world internals
// Does NOT generate CommandEnvelope or select actions
class ActionSpaceBuilder {
public:
    foundation::Result<ActionSpaceBuildResult> build(const ActionSpaceBuildRequest& request) const;
};

} // namespace pathfinder::agent
