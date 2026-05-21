#pragma once

#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"

namespace pathfinder::world_beast_ecology {

class BeastDecisionPolicy {
public:
    struct PolicyContext {
        std::string actor_key;
        BeastAgentProfile profile;
        std::vector<BeastPerceptionItem> perceptions;
        std::vector<pathfinder::knowledge::KnowledgeClaim> learned_claims;
        uint64_t tick = 0;
    };

    pathfinder::foundation::Result<BeastActionIntent> selectIntent(const PolicyContext& context);
};

} // namespace pathfinder::world_beast_ecology
