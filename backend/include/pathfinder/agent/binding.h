#pragma once

#include "pathfinder/agent/types.h"
#include "pathfinder/foundation/id.h"
#include <vector>
#include <optional>

namespace pathfinder::agent {

// AgentBinding: describes which Actor(s) an Agent controls
struct AgentBinding {
    AgentId agent_id;
    pathfinder::foundation::EntityId primary_actor_id;
    std::optional<pathfinder::foundation::TribeId> tribe_id;
    std::vector<pathfinder::foundation::EntityId> controlled_actor_ids;
    AgentControlAuthority authority = AgentControlAuthority::Unknown;

    // Validate all fields are set and consistent
    pathfinder::foundation::Result<void> validateBasic() const;
};

} // namespace pathfinder::agent
