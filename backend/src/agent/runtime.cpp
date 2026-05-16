#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/agent/action_space_builder.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/pipeline/context.h"
#include "pathfinder/pipeline/rule_pipeline.h"

namespace pathfinder::agent {

AgentRuntime::AgentRuntime(const AgentPolicy& policy)
    : policy_(policy) {}

foundation::Result<AgentTickResult> AgentRuntime::tickOne(
    const AgentTickRequest& request) const {

    // 1. Validate request
    auto req_result = request.validateBasic();
    if (req_result.is_error()) {
        return foundation::Result<AgentTickResult>::fail(req_result.errors());
    }

    AgentTickResult result;
    result.trace.phase_keys.push_back("request_validated");

    // 2. Build observation
    ObservationBuildRequest obs_req;
    obs_req.binding = request.binding;
    obs_req.state = request.state;
    obs_req.visibility = request.visibility;
    obs_req.include_cognition_claims = request.options.include_cognition_claims;

    ObservationBuilder obs_builder;
    auto obs_result = obs_builder.build(obs_req);
    if (obs_result.is_error()) {
        result.status = AgentRuntimeStatus::Failed;
        result.trace.phase_keys.push_back("observation_failed");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }
    result.observation = std::move(obs_result.value().observation);
    result.trace.phase_keys.push_back("observation_built");

    // 3. Build action space
    ActionSpaceBuildRequest as_req;
    as_req.observation = result.observation;
    as_req.include_explore_candidates = request.options.include_explore_candidates;
    as_req.include_wait_candidate = request.options.include_wait_candidate;

    ActionSpaceBuilder as_builder;
    auto as_result = as_builder.build(as_req);
    if (as_result.is_error()) {
        result.status = AgentRuntimeStatus::Failed;
        result.trace.phase_keys.push_back("action_space_failed");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }
    result.action_space = std::move(as_result.value().action_space);
    result.status = AgentRuntimeStatus::ActionSpaceBuilt;
    result.trace.phase_keys.push_back("action_space_built");

    // 4. Policy decision
    AgentPolicyRequest policy_req;
    policy_req.binding = request.binding;
    policy_req.observation = result.observation;
    policy_req.action_space = result.action_space;
    policy_req.decision_tick = request.issued_tick;
    policy_req.random_seed = request.random_seed;
    policy_req.submit_action_allowlist = request.options.submit_action_allowlist;

    auto decision_result = policy_.decide(policy_req);
    if (decision_result.is_error()) {
        result.status = AgentRuntimeStatus::NoDecision;
        result.trace.phase_keys.push_back("no_decision");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }
    result.decision = std::move(decision_result.value());
    result.status = AgentRuntimeStatus::DecisionMade;
    result.trace.phase_keys.push_back("decision_made");

    // Record trace
    for (size_t i = 0; i < result.action_space.candidates.size(); ++i) {
        if (result.action_space.candidates[i].action_id == result.decision.action_id ||
            result.action_space.candidates[i].command_action_id == result.decision.action_id) {
            result.trace.selected_candidate_index = static_cast<int>(i);
            result.trace.selected_candidate_id = result.action_space.candidates[i].action_id;
            result.trace.selected_command_action_id = result.action_space.candidates[i].command_action_id;
            break;
        }
    }

    // 5. Build command (if requested)
    if (!request.options.build_command) {
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }

    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(
        result.decision.selected_intent, request.command_source, request.issued_tick);
    if (cmd_result.is_error()) {
        result.status = AgentRuntimeStatus::SubmitSkipped;
        result.trace.phase_keys.push_back("command_adapter_rejected");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }
    result.command = std::move(cmd_result.value());
    result.status = AgentRuntimeStatus::CommandCreated;
    result.trace.command_created = true;
    result.trace.phase_keys.push_back("command_created");

    // 6. Submit to pipeline (if allowed)
    if (!request.options.submit_to_pipeline) {
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }

    // Check allowlist
    bool action_allowed = false;
    for (const auto& allowed : request.options.submit_action_allowlist) {
        if (allowed == result.decision.action_id) {
            action_allowed = true;
            break;
        }
    }
    if (!action_allowed) {
        result.status = AgentRuntimeStatus::SubmitSkipped;
        result.trace.phase_keys.push_back("submit_skipped");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }

    // 7. Execute RulePipeline
    pipeline::PipelineContext ctx;
    ctx.command = result.command.value();
    ctx.state_metadata = request.state->metadata;
    ctx.game_state = request.state;
    ctx.random_seed = request.random_seed;

    pipeline::RulePipeline pipeline;
    auto pipeline_result = pipeline.execute(ctx);
    if (pipeline_result.is_error()) {
        result.status = AgentRuntimeStatus::PipelineFailed;
        result.trace.phase_keys.push_back("pipeline_failed");
        return foundation::Result<AgentTickResult>::ok(std::move(result));
    }
    result.pipeline_result = std::move(pipeline_result.value());
    result.trace.pipeline_submitted = true;
    result.trace.phase_keys.push_back("pipeline_executed");

    if (result.pipeline_result->status == pipeline::PipelineStatus::Succeeded) {
        result.status = AgentRuntimeStatus::PipelineSucceeded;
    } else {
        result.status = AgentRuntimeStatus::PipelineFailed;
    }

    return foundation::Result<AgentTickResult>::ok(std::move(result));
}

} // namespace pathfinder::agent
