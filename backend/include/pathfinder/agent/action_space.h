#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/command/types.h"
#include "pathfinder/command/target.h"
#include <vector>
#include <string>

namespace pathfinder::agent {

// AgentActionCandidate: a single action the agent can choose
struct AgentActionCandidate {
    pathfinder::foundation::ActionId action_id;              // candidate instance id (e.g. eat_obj_xxx)
    pathfinder::foundation::ActionId command_action_id;      // semantic action id for CommandEnvelope (e.g. eat)
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTargetType> required_target_types;
    std::vector<command::ActionTarget> suggested_targets;    // pre-built targets for Policy to copy into Intent
    bool command_supported = false;
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
