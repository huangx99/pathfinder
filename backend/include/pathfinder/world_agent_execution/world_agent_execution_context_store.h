#pragma once

#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include <unordered_map>
#include <mutex>

namespace pathfinder::world_agent_execution {

class InMemoryWorldAgentExecutionContextStore : public IWorldAgentExecutionContextStore {
public:
    pathfinder::foundation::Result<std::optional<pathfinder::goal_execution::ExecutionContext>> findByActor(const std::string& actor_key) const override;
    pathfinder::foundation::Result<void> saveContext(const pathfinder::goal_execution::ExecutionContext& context) override;
    pathfinder::foundation::Result<void> clearActor(const std::string& actor_key) override;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, pathfinder::goal_execution::ExecutionContext> store_;
};

} // namespace pathfinder::world_agent_execution
