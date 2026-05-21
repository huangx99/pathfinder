#pragma once

#include "pathfinder/world_command/world_command_handler.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/knowledge/knowledge_repository.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include <memory>

namespace pathfinder::world_teaching {

// ---------------------------------------------------------------------------
// TeachCommandHandler
// ---------------------------------------------------------------------------
// Handles WorldCommandKind::Teach.
// Integrates eligibility, propagation, store, and projection.
// ---------------------------------------------------------------------------

class TeachCommandHandler : public world_command::IWorldCommandHandler {
public:
    TeachCommandHandler(
        pathfinder::knowledge::KnowledgeRepository& repository,
        std::unique_ptr<IWorldActorQueryPort> actor_query_port);

    world_command::WorldCommandKind kind() const override;

    pathfinder::foundation::Result<world_command::WorldCommandExecutionResult> execute(
        world_command::WorldCommandContext& context,
        const world_command::WorldCommandDto& command) const override;

private:
    pathfinder::knowledge::KnowledgeRepository& repository_;
    std::unique_ptr<IWorldActorQueryPort> actor_query_port_;
};

std::shared_ptr<world_command::IWorldCommandHandler> createTeachCommandHandler(
    pathfinder::knowledge::KnowledgeRepository& repository,
    std::unique_ptr<IWorldActorQueryPort> actor_query_port);

} // namespace pathfinder::world_teaching
