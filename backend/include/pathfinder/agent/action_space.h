#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/command/types.h"
#include <vector>
#include <string>

namespace pathfinder::agent {

// AgentActionCandidate: a single action the agent can choose
struct AgentActionCandidate {
    pathfinder::foundation::ActionId action_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTargetType> required_target_types;
    bool command_supported = false;  // whether this can be converted to CommandEnvelope
    std::string reason_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// AgentActionSpace: the set of available actions for an agent
struct AgentActionSpace {
    AgentId agent_id;
    pathfinder::foundation::StateVersion state_version;
    std::vector<AgentActionCandidate> candidates;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
