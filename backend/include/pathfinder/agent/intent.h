#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/foundation/id.h"
#include "pathfinder/foundation/version.h"
#include "pathfinder/command/target.h"
#include <vector>
#include <string>

namespace pathfinder::agent {

// AgentIntent: a decision made by an agent to perform an action
struct AgentIntent {
    AgentId agent_id;
    pathfinder::foundation::DecisionId decision_id;
    pathfinder::foundation::EntityId actor_id;
    AgentIntentType intent_type = AgentIntentType::Unknown;
    std::vector<command::ActionTarget> targets;
    double confidence = 0.0;  // [0.0, 1.0]
    std::string reason_key;
    pathfinder::foundation::ActionId action_id;  // semantic action type (e.g. "eat"), set from ActionSpace candidate
    bool command_supported_snapshot = true;  // snapshot from ActionSpace candidate's command_supported

    pathfinder::foundation::Result<void> validateBasic() const;
};

// AgentDecision: a record of the decision process
struct AgentDecision {
    pathfinder::foundation::DecisionId decision_id;
    AgentId agent_id;
    AgentIntent selected_intent;
    pathfinder::foundation::StateVersion observation_state_version;
    pathfinder::foundation::ActionId action_id;
    std::string reason_key;

    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
