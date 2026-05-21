#pragma once

#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// WorldTeachingStorePort
// ---------------------------------------------------------------------------
// Saves recipient claims after successful propagation.
// Does not modify world runtime.
// ---------------------------------------------------------------------------

class WorldTeachingStorePort {
public:
    explicit WorldTeachingStorePort(pathfinder::knowledge::KnowledgeRepository& repository);

    pathfinder::foundation::Result<void> putClaims(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims);

private:
    pathfinder::knowledge::KnowledgeRepository& repository_;
};

} // namespace pathfinder::world_teaching
