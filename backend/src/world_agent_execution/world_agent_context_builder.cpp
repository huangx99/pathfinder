#include "pathfinder/world_agent_execution/world_agent_context_builder.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

WorldAgentContextBuilder::WorldAgentContextBuilder(
    IAgentVisibleWorldQueryPort& visible_world_port,
    IAgentInventoryQueryPort& inventory_port,
    IAgentKnowledgeQueryPort& knowledge_port,
    IWorldAgentExecutionContextStore& context_store)
    : visible_world_port_(visible_world_port)
    , inventory_port_(inventory_port)
    , knowledge_port_(knowledge_port)
    , context_store_(context_store) {
}

Result<WorldAgentDecisionContext> WorldAgentContextBuilder::build(const WorldAgentTickRequest& request) {
    auto actor_result = visible_world_port_.findActor(request.actor_key);
    if (actor_result.is_error()) {
        return Result<WorldAgentDecisionContext>::fail(makeError(ErrorCode::precondition_actor_unavailable, "context_builder.actor_not_found"));
    }
    auto actor = actor_result.value();

    auto snapshot_result = visible_world_port_.visibleSnapshotForActor(request.actor_key);
    if (snapshot_result.is_error()) {
        return Result<WorldAgentDecisionContext>::fail(makeError(ErrorCode::state_snapshot_missing, "context_builder.snapshot_failed"));
    }

    auto inventory_result = inventory_port_.snapshotForActorDecision(request.actor_key);
    if (inventory_result.is_error()) {
        return Result<WorldAgentDecisionContext>::fail(makeError(ErrorCode::state_snapshot_missing, "context_builder.inventory_failed"));
    }

    auto claims_result = knowledge_port_.claimsForActor(request.actor_key);
    if (claims_result.is_error()) {
        return Result<WorldAgentDecisionContext>::fail(makeError(ErrorCode::knowledge_not_found, "context_builder.knowledge_failed"));
    }

    auto resources_result = visible_world_port_.visibleResourcesForActor(request.actor_key);
    auto entities_result = visible_world_port_.visibleEntitiesForActor(request.actor_key);

    auto existing_context_result = context_store_.findByActor(request.actor_key);
    std::optional<pathfinder::goal_execution::ExecutionContext> existing_context;
    if (existing_context_result.is_ok() && existing_context_result.value().has_value()) {
        existing_context = existing_context_result.value().value();
    }

    WorldAgentDecisionContext ctx;
    ctx.context_id = request.request_id + ".ctx";
    ctx.actor_key = request.actor_key;
    ctx.actor = actor;
    ctx.runtime_snapshot = snapshot_result.value();
    ctx.inventory_snapshot = inventory_result.value();
    ctx.actor_claims = claims_result.value();
    if (resources_result.is_ok()) {
        ctx.visible_resources = resources_result.value();
    }
    if (entities_result.is_ok()) {
        ctx.visible_entities = entities_result.value();
    }
    ctx.existing_execution_context = existing_context;
    ctx.external_interrupts = request.external_interrupts;
    ctx.tick = request.tick;

    return Result<WorldAgentDecisionContext>::ok(ctx);
}

} // namespace pathfinder::world_agent_execution
