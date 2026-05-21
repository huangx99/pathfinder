#pragma once

#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// WorldTeachingEligibilityService
// ---------------------------------------------------------------------------
// Validates teacher, recipient, distance, source claim, and teachable flag.
// Does NOT execute propagation.
// ---------------------------------------------------------------------------

class WorldTeachingEligibilityService {
public:
    WorldTeachingEligibilityResult check(
        const WorldTeachingRequest& request,
        const pathfinder::knowledge::KnowledgeRepository& repository,
        const IWorldActorQueryPort& actor_query_port) const;
};

} // namespace pathfinder::world_teaching
