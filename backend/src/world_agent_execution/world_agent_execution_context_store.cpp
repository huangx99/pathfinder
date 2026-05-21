#include "pathfinder/world_agent_execution/world_agent_execution_context_store.h"

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::Result;

Result<std::optional<pathfinder::goal_execution::ExecutionContext>> InMemoryWorldAgentExecutionContextStore::findByActor(const std::string& actor_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(actor_key);
    if (it != store_.end()) {
        return Result<std::optional<pathfinder::goal_execution::ExecutionContext>>::ok(it->second);
    }
    return Result<std::optional<pathfinder::goal_execution::ExecutionContext>>::ok(std::nullopt);
}

Result<void> InMemoryWorldAgentExecutionContextStore::saveContext(const pathfinder::goal_execution::ExecutionContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    store_[context.actor_key] = context;
    return Result<void>::ok();
}

Result<void> InMemoryWorldAgentExecutionContextStore::clearActor(const std::string& actor_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    store_.erase(actor_key);
    return Result<void>::ok();
}

} // namespace pathfinder::world_agent_execution
