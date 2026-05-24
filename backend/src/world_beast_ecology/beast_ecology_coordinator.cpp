#include "pathfinder/world_beast_ecology/beast_ecology_coordinator.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_beast_ecology {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandExecutionResult;
using pathfinder::world_command::WorldEventDto;
using pathfinder::world_command::WorldStateDeltaDto;
using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::goal_execution::ExternalInterruptSignal;

BeastEcologyCoordinator::BeastEcologyCoordinator(
    BeastPerceptionBuilder& perception_builder,
    BeastInstinctGate& instinct_gate,
    BeastDecisionPolicy& decision_policy,
    BeastCommandCompiler& command_compiler,
    BeastInterruptProjector& interrupt_projector,
    BeastProjectionBridge& projection_bridge,
    IBeastProfileQueryPort& profile_port,
    IBeastWorldQueryPort& world_port,
    IBeastKnowledgeQueryPort& knowledge_port,
    IBeastCommandPipelinePort& pipeline_port)
    : perception_builder_(perception_builder)
    , instinct_gate_(instinct_gate)
    , decision_policy_(decision_policy)
    , command_compiler_(command_compiler)
    , interrupt_projector_(interrupt_projector)
    , projection_bridge_(projection_bridge)
    , profile_port_(profile_port)
    , world_port_(world_port)
    , knowledge_port_(knowledge_port)
    , pipeline_port_(pipeline_port) {
}

Result<BeastTickResult> BeastEcologyCoordinator::makeFailureResult(
    const BeastTickRequest& request,
    const std::vector<std::string>& reason_keys) {
    BeastTickResult result;
    result.ok = false;
    result.actor_key = request.actor_key;
    result.reason_keys = reason_keys;
    return Result<BeastTickResult>::ok(result);
}

Result<BeastTickResult> BeastEcologyCoordinator::tick(const BeastTickRequest& request) {
    // 1. Validate request
    auto validate_result = request.validateBasic();
    if (validate_result.is_error()) {
        return makeFailureResult(request, {"request_validation_failed"});
    }

    BeastTickResult result;
    result.actor_key = request.actor_key;

    // 2. Query profile
    auto profile_result = profile_port_.profileForActor(request.actor_key);
    if (profile_result.is_error()) {
        return makeFailureResult(request, {"profile_query_failed"});
    }
    auto profile = profile_result.value();

    // 3. Query actor
    auto actor_result = world_port_.findBeastActor(request.actor_key);
    if (actor_result.is_error()) {
        return makeFailureResult(request, {"actor_not_found"});
    }
    auto actor = actor_result.value();

    // 4. Query raw world data and build perception via builder
    auto nearby_actors_result = world_port_.nearbyActorsForBeast(request.actor_key);
    auto nearby_entities_result = world_port_.nearbyEntitiesForBeast(request.actor_key);
    auto nearby_resources_result = world_port_.nearbyResourcesForBeast(request.actor_key);
    auto effects_result = world_port_.nearbyEffectsForBeast(request.actor_key);

    if (nearby_actors_result.is_error() || nearby_entities_result.is_error() ||
        nearby_resources_result.is_error() || effects_result.is_error()) {
        return makeFailureResult(request, {"world_query_failed"});
    }

    auto perceptions_result = perception_builder_.build(
        request.actor_key,
        profile,
        actor,
        nearby_actors_result.value(),
        nearby_entities_result.value(),
        nearby_resources_result.value(),
        effects_result.value());

    if (perceptions_result.is_error()) {
        return makeFailureResult(request, {"perception_build_failed"});
    }
    auto perceptions = perceptions_result.value();

    // 5. Query beast knowledge
    std::vector<pathfinder::knowledge::KnowledgeClaim> claims;
    if (request.allow_learning_claims) {
        auto claims_result = knowledge_port_.claimsForBeast(request.actor_key);
        if (claims_result.is_ok()) {
            claims = claims_result.value();
        }
    }

    // 6. Decision policy
    BeastDecisionPolicy::PolicyContext policy_context;
    policy_context.actor_key = request.actor_key;
    policy_context.actor_coord = actor.coord;
    policy_context.profile = profile;
    policy_context.perceptions = perceptions;
    policy_context.learned_claims = claims;
    policy_context.tick = request.tick;

    auto intent_result = decision_policy_.selectIntent(policy_context);
    if (intent_result.is_error()) {
        return makeFailureResult(request, {"decision_policy_failed"});
    }
    auto intent = intent_result.value();
    result.selected_intent = intent;

    // 7. Instinct gate
    auto gate_result = instinct_gate_.check(profile, intent);
    if (!gate_result.allowed) {
        intent.kind = BeastActionIntentKind::Wait;
        intent.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
        intent.reason_kind = BeastDecisionReasonKind::CommandBlocked;
        intent.risk_score = 0.0;
        intent.target_ref.clear();
        intent.target_key.clear();
        intent.target_coord.reset();
        intent.safe_summary_zh_cn = "instinct_blocked_wait";
        result.selected_intent = intent;
        result.reason_keys.push_back(gate_result.blocker_key);
    }

    // 8. Compile command
    auto command_result = command_compiler_.compile(intent, request.request_id, request.actor_key);
    if (command_result.is_error()) {
        return makeFailureResult(request, {"command_compile_failed"});
    }
    auto command = command_result.value();
    result.issued_command = command;

    // 9. Execute or dry run
    if (request.allow_issue_command) {
        auto pipeline_result = pipeline_port_.executeBeastCommand(command);
        if (pipeline_result.is_ok()) {
            result.command_result = pipeline_result.value();
            result.events = pipeline_result.value().events;
            result.state_deltas = pipeline_result.value().state_deltas;
            if (pipeline_result.value().projection_patch_override.has_value()) {
                result.projection_patch = pipeline_result.value().projection_patch_override.value();
            }
        } else {
            result.reason_keys.push_back("pipeline_execution_failed");
        }
    }

    // 10. Project external interrupts
    result.interrupts = interrupt_projector_.project(intent, perceptions, request.tick);

    // 11. Finalize
    result.ok = true;
    result.reason_keys.push_back(toString(intent.reason_kind));
    return Result<BeastTickResult>::ok(result);
}

} // namespace pathfinder::world_beast_ecology
