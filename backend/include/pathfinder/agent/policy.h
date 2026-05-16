#pragma once

#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/observation.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/foundation/time.h"
#include "pathfinder/foundation/random.h"
#include "pathfinder/foundation/result.h"
#include <string>

namespace pathfinder::agent {

// AgentPolicyRequest: input for policy decision (no world state access)
struct AgentPolicyRequest {
    AgentBinding binding;
    AgentObservation observation;
    AgentActionSpace action_space;
    pathfinder::foundation::Tick decision_tick;
    pathfinder::foundation::RandomSeed random_seed;
    std::string policy_reason_prefix;
    std::vector<pathfinder::foundation::ActionId> submit_action_allowlist;

    pathfinder::foundation::Result<void> validateBasic() const;
};

// AgentPolicy: abstract policy interface
class AgentPolicy {
public:
    virtual ~AgentPolicy() = default;
    virtual pathfinder::foundation::Result<AgentDecision> decide(
        const AgentPolicyRequest& request) const = 0;
};

// FirstSupportedPolicy: selects first command_supported=true candidate
class FirstSupportedPolicy : public AgentPolicy {
public:
    pathfinder::foundation::Result<AgentDecision> decide(
        const AgentPolicyRequest& request) const override;
};

} // namespace pathfinder::agent
