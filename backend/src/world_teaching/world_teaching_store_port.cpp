#include "pathfinder/world_teaching/world_teaching_store_port.h"

namespace pathfinder::world_teaching {

WorldTeachingStorePort::WorldTeachingStorePort(pathfinder::knowledge::KnowledgeRepository& repository)
    : repository_(repository) {}

pathfinder::foundation::Result<void> WorldTeachingStorePort::putClaims(
    const std::vector<pathfinder::knowledge::KnowledgeClaim>& claims) {
    for (const auto& claim : claims) {
        auto res = repository_.put(claim);
        if (!res.is_ok()) return res;
    }
    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::world_teaching
