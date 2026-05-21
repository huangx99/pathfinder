#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"

namespace pathfinder::world_teaching {

pathfinder::knowledge::KnowledgeOwner WorldActorKnowledgeOwnerResolver::resolve(
    const std::string& actor_key) const {
    pathfinder::knowledge::KnowledgeOwner owner;
    owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    return owner;
}

} // namespace pathfinder::world_teaching
