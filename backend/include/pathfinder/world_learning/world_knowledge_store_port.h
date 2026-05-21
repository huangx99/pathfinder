#pragma once

#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_learning {

// ---------------------------------------------------------------------------
// WorldKnowledgeStorePort
// ---------------------------------------------------------------------------
// Adapter port for querying and writing actor knowledge claims.
// Does not use global static state.
// ---------------------------------------------------------------------------

class WorldKnowledgeStorePort {
public:
    explicit WorldKnowledgeStorePort(pathfinder::knowledge::KnowledgeRepository& repository);

    pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>> queryByOwner(
        const std::string& actor_key) const;

    pathfinder::foundation::Result<void> putClaim(
        const pathfinder::knowledge::KnowledgeClaim& claim);

    pathfinder::foundation::Result<void> putClaims(
        const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims);

    size_t size() const;

private:
    pathfinder::knowledge::KnowledgeRepository& repository_;
};

} // namespace pathfinder::world_learning
