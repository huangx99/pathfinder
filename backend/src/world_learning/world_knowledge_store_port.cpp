#include "pathfinder/world_learning/world_knowledge_store_port.h"

namespace pathfinder::world_learning {

WorldKnowledgeStorePort::WorldKnowledgeStorePort(pathfinder::knowledge::KnowledgeRepository& repository)
    : repository_(repository) {}

pathfinder::foundation::Result<std::vector<pathfinder::knowledge::KnowledgeClaim>>
WorldKnowledgeStorePort::queryByOwner(const std::string& actor_key) const {
    pathfinder::knowledge::KnowledgeQuery query;
    query.owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    query.owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    query.mode = pathfinder::knowledge::KnowledgeQueryMode::ByOwner;
    return repository_.query(query);
}

pathfinder::foundation::Result<void> WorldKnowledgeStorePort::putClaim(
    const pathfinder::knowledge::KnowledgeClaim& claim) {
    return repository_.put(claim);
}

pathfinder::foundation::Result<void> WorldKnowledgeStorePort::putClaims(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    for (const auto& claim : claims) {
        auto res = repository_.put(claim);
        if (!res.is_ok()) return res;
    }
    return pathfinder::foundation::Result<void>::ok();
}

size_t WorldKnowledgeStorePort::size() const {
    return repository_.size();
}

} // namespace pathfinder::world_learning
