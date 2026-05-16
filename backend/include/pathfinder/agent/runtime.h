#pragma once

#include "pathfinder/agent/policy.h"
#include "pathfinder/agent/runtime_types.h"

namespace pathfinder::agent {

// AgentRuntime: orchestrates observe -> decide -> adapt -> submit
class AgentRuntime {
public:
    explicit AgentRuntime(const AgentPolicy& policy);

    foundation::Result<AgentTickResult> tickOne(const AgentTickRequest& request) const;

private:
    const AgentPolicy& policy_;
};

} // namespace pathfinder::agent
