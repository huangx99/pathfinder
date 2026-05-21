#include "pathfinder/world_agent_execution/agent_execution_coordinator.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::foundation::Result;
using pathfinder::agent_reasoning::AgentPlanStep;
using pathfinder::goal_execution::ExecutionContext;
using pathfinder::goal_execution::GoalFrame;
using pathfinder::goal_execution::ExecutionFrameStatus;
using pathfinder::goal_execution::InternalBlocker;
using pathfinder::goal_execution::InternalBlockerKind;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandSource;

AgentExecutionCoordinator::AgentExecutionCoordinator(
    WorldAgentContextBuilder& context_builder,
    WorldAgentGoalSelector& goal_selector,
    WorldAgentReasoningBridge& reasoning_bridge,
    AgentActionKnowledgeGuard& knowledge_guard,
    AgentPlanCommandCompiler& command_compiler,
    IAgentCommandPipelinePort& pipeline_port,
    IWorldAgentExecutionContextStore& context_store,
    WorldAgentProjectionBridge& projection_bridge)
    : context_builder_(context_builder)
    , goal_selector_(goal_selector)
    , reasoning_bridge_(reasoning_bridge)
    , knowledge_guard_(knowledge_guard)
    , command_compiler_(command_compiler)
    , pipeline_port_(pipeline_port)
    , context_store_(context_store)
    , projection_bridge_(projection_bridge) {
}

Result<WorldAgentTickResult> AgentExecutionCoordinator::makeFailureResult(
    const WorldAgentTickRequest& request,
    WorldAgentExecutionFailureKind failure_kind,
    const std::vector<std::string>& reason_keys) {
    WorldAgentTickResult result;
    result.ok = false;
    result.decision = WorldAgentExecutionDecision::Unknown;
    result.failure_kind = failure_kind;
    result.actor_key = request.actor_key;
    result.reason_keys = reason_keys;
    return Result<WorldAgentTickResult>::ok(result);
}

Result<WorldAgentTickResult> AgentExecutionCoordinator::tick(const WorldAgentTickRequest& request) {
    // 1. Validate request
    auto validate_result = request.validateBasic();
    if (validate_result.is_error()) {
        return makeFailureResult(request, WorldAgentExecutionFailureKind::InvalidRequest, {"request_validation_failed"});
    }

    // 2. Build context
    auto context_result = context_builder_.build(request);
    if (context_result.is_error()) {
        return makeFailureResult(request, WorldAgentExecutionFailureKind::ContextBuildFailed, {"context_build_failed"});
    }
    auto context = context_result.value();

    // 3. Check max depth
    uint32_t current_depth = 0;
    if (context.existing_execution_context.has_value()) {
        current_depth = static_cast<uint32_t>(context.existing_execution_context.value().goal_stack.size());
    }
    if (current_depth >= request.max_subgoal_depth) {
        return makeFailureResult(request, WorldAgentExecutionFailureKind::MaxDepthExceeded, {"max_subgoal_depth_exceeded"});
    }

    // 4. Initialize result
    WorldAgentTickResult result;
    result.actor_key = request.actor_key;
    result.execution_context_after = context.existing_execution_context.value_or(ExecutionContext{});
    result.execution_context_after.actor_key = request.actor_key;
    result.execution_context_after.last_update_tick = request.tick;

    // 5. Select goal
    auto selection_result = goal_selector_.select(context, request);
    if (selection_result.is_error() || !selection_result.value().has_goal) {
        result.ok = true;
        result.decision = WorldAgentExecutionDecision::NoopNoGoal;
        result.reason_keys.push_back("no_goal_selected");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }
    auto selection = selection_result.value();
    result.selected_goal = selection.goal;
    result.decision = WorldAgentExecutionDecision::SelectedGoal;

    // 6. Reason / plan
    auto reasoning_result = reasoning_bridge_.reasonForGoal(context, selection.goal);
    if (reasoning_result.is_error() || !reasoning_result.value().ok || !reasoning_result.value().plan.has_value()) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::ReasoningFailed;
        result.reason_keys.push_back("reasoning_failed");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }
    auto plan = reasoning_result.value().plan.value();
    result.selected_plan = plan;

    if (plan.steps.empty()) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::PlanEmpty;
        result.reason_keys.push_back("plan_empty");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }

    // 7. Select next step
    const auto& step = plan.steps[0];
    result.selected_step = step;
    result.decision = WorldAgentExecutionDecision::PlannedStep;

    // 8. Knowledge guard
    auto guard_result = knowledge_guard_.checkStep(
        request.actor_key, step, context.actor_claims,
        request.allow_hypothesis_knowledge, request.allow_risk_action);
    if (guard_result.is_error() || !guard_result.value().allowed) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::KnowledgeMissing;
        result.decision = WorldAgentExecutionDecision::WaitingForKnowledge;
        InternalBlocker blocker;
        blocker.kind = InternalBlockerKind::KnowledgeMissing;
        blocker.actor_key = request.actor_key;
        blocker.missing_key = step.action_key;
        blocker.safe_summary_zh_cn = "knowledge_missing_for_action";
        result.blockers.push_back(blocker);
        result.reason_keys.push_back("knowledge_guard_blocked");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }

    // 9. Compile command
    auto compile_result = command_compiler_.compile(request.actor_key, step, context, request.request_id);
    if (compile_result.is_error()) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::CommandCompileFailed;
        result.reason_keys.push_back("command_compile_failed");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }
    auto compiled = compile_result.value();
    if (!compiled.ok) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::CommandCompileFailed;
        result.blockers = compiled.blockers;
        result.decision = WorldAgentExecutionDecision::CommandBlocked;
        result.reason_keys.push_back("command_compile_blocker");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }

    result.issued_command = compiled.command;
    result.decision = WorldAgentExecutionDecision::IssuedCommand;

    // 10. Dry run check
    if (!request.allow_issue_command) {
        result.ok = true;
        result.reason_keys.push_back("dry_run");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }

    // 11. Submit via pipeline
    auto pipeline_result = pipeline_port_.executeAgentCommand(compiled.command);
    if (pipeline_result.is_error()) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::CommandPipelineFailed;
        result.reason_keys.push_back("pipeline_failed");
        context_store_.saveContext(result.execution_context_after);
        return Result<WorldAgentTickResult>::ok(result);
    }
    auto cmd_result = pipeline_result.value();
    result.command_result = cmd_result;

    // Merge events and deltas
    result.events = cmd_result.events;
    result.state_deltas = cmd_result.state_deltas;
    result.projection_patch = cmd_result.projection_patch_override;

    // Determine decision from result
    switch (cmd_result.result_kind) {
        case pathfinder::world_command::WorldCommandResultKind::Succeeded:
            result.decision = WorldAgentExecutionDecision::CommandSucceeded;
            result.ok = true;
            break;
        case pathfinder::world_command::WorldCommandResultKind::Blocked:
            result.decision = WorldAgentExecutionDecision::CommandBlocked;
            result.failure_kind = WorldAgentExecutionFailureKind::CommandBlocked;
            result.ok = false;
            break;
        default:
            result.decision = WorldAgentExecutionDecision::CommandFailed;
            result.failure_kind = WorldAgentExecutionFailureKind::CommandPipelineFailed;
            result.ok = false;
            break;
    }

    // 12. Update execution context
    GoalFrame frame;
    frame.frame_id = request.request_id + ".frame";
    frame.actor_key = request.actor_key;
    frame.goal_id = selection.goal.goal_id;
    frame.plan_id = plan.plan_id;
    frame.status = result.ok ? ExecutionFrameStatus::Running : ExecutionFrameStatus::Failed;
    frame.created_tick = request.tick;

    result.execution_context_after.goal_stack.push_back(frame);
    result.execution_context_after.active_frame_id = frame.frame_id;

    // 13. Store context
    auto store_result = context_store_.saveContext(result.execution_context_after);
    if (store_result.is_error()) {
        result.ok = false;
        result.failure_kind = WorldAgentExecutionFailureKind::InvalidRequest;
        result.reason_keys.push_back("context_store_failed");
    }

    // 14. Build projection (MVP: minimal)
    result.reason_keys.push_back("tick_complete");
    return Result<WorldAgentTickResult>::ok(result);
}

} // namespace pathfinder::world_agent_execution
