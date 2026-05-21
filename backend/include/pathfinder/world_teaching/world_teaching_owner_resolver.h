#pragma once

#include "pathfinder/knowledge/knowledge_claim.h"
#include <string>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// WorldActorKnowledgeOwnerResolver
// ---------------------------------------------------------------------------
// Maps actor_key to KnowledgeOwner without creating global knowledge.
// ---------------------------------------------------------------------------

class WorldActorKnowledgeOwnerResolver {
public:
    pathfinder::knowledge::KnowledgeOwner resolve(const std::string& actor_key) const;
};

} // namespace pathfinder::world_teaching
