#include "pathfinder/world_teaching/teach_command_handler.h"
#include "pathfinder/world_teaching/world_teaching_types.h"
#include "pathfinder/world_teaching/world_teaching_owner_resolver.h"
#include "pathfinder/world_teaching/world_teaching_eligibility_service.h"
#include "pathfinder/world_teaching/world_teaching_loop_bridge.h"
#include "pathfinder/world_teaching/world_teaching_store_port.h"
#include "pathfinder/world_teaching/world_teaching_projection_bridge.h"
#include "pathfinder/foundation/error.h"

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;

namespace pathfinder::world_teaching {

TeachCommandHandler::TeachCommandHandler(
    pathfinder::knowledge::KnowledgeRepository& repository,
    std::unique_ptr<IWorldActorQueryPort> actor_query_port,
    const pathfinder::content::ContentRegistry* content_registry)
    : repository_(repository), actor_query_port_(std::move(actor_query_port)), content_registry_(content_registry) {}

world_command::WorldCommandKind TeachCommandHandler::kind() const {
    return world_command::WorldCommandKind::Teach;
}

Result<world_command::WorldCommandExecutionResult> TeachCommandHandler::execute(
    world_command::WorldCommandContext& /*context*/,
    const world_command::WorldCommandDto& command) const {

    world_command::WorldCommandExecutionResult result;
    result.result_kind = world_command::WorldCommandResultKind::Failed;

    if (!actor_query_port_) {
        result.result_kind = world_command::WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back(toString(WorldTeachingFailureKind::InvalidRequest));
        return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

    // Parse command
    if (command.target.target_kind != world_command::WorldCommandTargetKind::Actor &&
        command.target.target_kind != world_command::WorldCommandTargetKind::Knowledge) {
        result.result_kind = world_command::WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back(toString(WorldTeachingFailureKind::InvalidRequest));
        return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

    std::string teacher_actor_key = command.actor_key;
    std::string recipient_actor_key = command.target.target_actor_key;
    std::string source_knowledge_id_str = command.target.knowledge_key;

    if (teacher_actor_key.empty() || recipient_actor_key.empty() || source_knowledge_id_str.empty()) {
        result.result_kind = world_command::WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back(toString(WorldTeachingFailureKind::InvalidRequest));
        return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

    pathfinder::foundation::KnowledgeId source_knowledge_id(source_knowledge_id_str);

    // Build request
    WorldTeachingRequest request;
    request.request_id = command.command_id;
    request.source_command = command;
    request.teacher_actor_key = teacher_actor_key;
    request.recipient_actor_key = recipient_actor_key;
    request.source_knowledge_id = source_knowledge_id;
    request.tick = command.context.issued_tick;
    request.channel = pathfinder::knowledge::KnowledgePropagationChannel::DirectTeaching;
    request.trust_band = pathfinder::knowledge::KnowledgePropagationTrustBand::High;
    request.trust_score = 0.8;

    // Load teacher and recipient claims from repository
    WorldActorKnowledgeOwnerResolver resolver;
    auto teacher_owner = resolver.resolve(teacher_actor_key);
    auto recipient_owner = resolver.resolve(recipient_actor_key);

    auto teacher_claims_res = repository_.listByOwner(teacher_owner);
    if (teacher_claims_res.is_ok()) {
        request.teacher_claims = teacher_claims_res.value();
    }

    auto recipient_claims_res = repository_.listByOwner(recipient_owner);
    if (recipient_claims_res.is_ok()) {
        request.recipient_claims = recipient_claims_res.value();
    }

    // 1. Eligibility check
    WorldTeachingEligibilityService eligibility_service;
    auto eligibility = eligibility_service.check(request, repository_, *actor_query_port_);

    if (!eligibility.allowed) {
        result.result_kind = world_command::WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back(toString(eligibility.failure_kind));
        return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

    // 2. Propagation via loop bridge
    WorldTeachingLoopBridge loop_bridge;
    auto bridge_result = loop_bridge.propagate(
        eligibility.source_claim.value(),
        eligibility.recipient_owner,
        request.recipient_claims,
        request.channel,
        request.trust_band,
        request.trust_score,
        pathfinder::foundation::Tick(request.tick),
        command.command_id + "_teach_loop");

    if (!bridge_result.ok) {
        result.result_kind = world_command::WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back(toString(bridge_result.failure_kind));
        return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

    // 3. Store recipient claims
    WorldTeachingStorePort store_port(repository_);
    std::vector<pathfinder::knowledge::KnowledgeClaim> claims_to_store;
    claims_to_store.insert(claims_to_store.end(), bridge_result.recipient_created_claims.begin(), bridge_result.recipient_created_claims.end());
    claims_to_store.insert(claims_to_store.end(), bridge_result.recipient_updated_claims.begin(), bridge_result.recipient_updated_claims.end());

    if (!claims_to_store.empty()) {
        auto store_r = store_port.putClaims(claims_to_store);
        if (!store_r.is_ok()) {
            result.result_kind = world_command::WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back(toString(WorldTeachingFailureKind::StoreFailed));
            return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
        }
    }

    // 4. Build projection
    WorldTeachingProjectionBridge projection_bridge(content_registry_);
    auto projection = projection_bridge.project(
        claims_to_store,
        eligibility.source_claim.has_value() ? &eligibility.source_claim.value() : nullptr,
        teacher_actor_key,
        recipient_actor_key,
        source_knowledge_id_str,
        bridge_result.decision,
        request.tick);

    result.result_kind = world_command::WorldCommandResultKind::Succeeded;
    result.events = std::move(projection.events);
    result.state_deltas = std::move(projection.state_deltas);
    result.projection_patch_override = std::move(projection.patch);

    return Result<world_command::WorldCommandExecutionResult>::ok(std::move(result));
}

std::shared_ptr<world_command::IWorldCommandHandler> createTeachCommandHandler(
    pathfinder::knowledge::KnowledgeRepository& repository,
    std::unique_ptr<IWorldActorQueryPort> actor_query_port,
    const pathfinder::content::ContentRegistry* content_registry) {
    return std::make_shared<TeachCommandHandler>(repository, std::move(actor_query_port), content_registry);
}

} // namespace pathfinder::world_teaching
