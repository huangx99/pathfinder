#pragma once

#include "pathfinder/agent/intent.h"
#include "pathfinder/command/envelope.h"
#include "pathfinder/foundation/time.h"

namespace pathfinder::agent {

// AgentCommandAdapter: converts AgentIntent to CommandEnvelope
// Does NOT execute rules or access mutable world internals
class AgentCommandAdapter {
public:
    // Convert an AgentIntent to a CommandEnvelope
    // Returns error for unsupported intent types (Wait, CallGroup)
    foundation::Result<command::CommandEnvelope> toCommandEnvelope(
        const AgentIntent& intent,
        command::CommandSource source,
        pathfinder::foundation::Tick issued_tick) const;

private:
    // Deterministic ID generation from decision_id
    static pathfinder::foundation::CommandId makeCommandIdFromDecision(
        const pathfinder::foundation::DecisionId& decision_id);
    static pathfinder::foundation::ActionId makeActionIdFromDecision(
        const pathfinder::foundation::DecisionId& decision_id);
};

} // namespace pathfinder::agent
