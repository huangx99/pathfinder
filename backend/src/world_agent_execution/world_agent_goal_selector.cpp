#include "pathfinder/world_agent_execution/world_agent_goal_selector.h"

namespace pathfinder::world_agent_execution {

using pathfinder::foundation::Result;
using pathfinder::agent_reasoning::AgentGoal;
using pathfinder::agent_reasoning::AgentGoalKind;
using pathfinder::goal_execution::ExecutionContext;
using pathfinder::goal_execution::ExternalInterruptKind;

WorldAgentGoalSelector::WorldAgentGoalSelector() {
}

bool WorldAgentGoalSelector::hasActiveGoal(const ExecutionContext& exec_ctx) const {
    if (!exec_ctx.active_frame_id.has_value()) return false;
    for (const auto& frame : exec_ctx.goal_stack) {
        if (frame.frame_id == exec_ctx.active_frame_id.value()) {
            return frame.status == pathfinder::goal_execution::ExecutionFrameStatus::Running ||
                   frame.status == pathfinder::goal_execution::ExecutionFrameStatus::Pending ||
                   frame.status == pathfinder::goal_execution::ExecutionFrameStatus::WaitingForReasoning;
        }
    }
    return false;
}

Result<WorldAgentGoalSelector::SelectionResult> WorldAgentGoalSelector::selectFromInterrupts(
    const WorldAgentDecisionContext& context) {
    SelectionResult result;
    for (const auto& interrupt : context.external_interrupts) {
        if (interrupt.kind == ExternalInterruptKind::ThreatAppeared ||
            interrupt.kind == ExternalInterruptKind::ThreatEscalated ||
            interrupt.kind == ExternalInterruptKind::CommandOverride) {
            result.has_goal = true;
            result.goal.goal_id = "emergency." + interrupt.interrupt_id;
            result.goal.actor_key = context.actor_key;
            result.goal.kind = AgentGoalKind::ReduceThreat;
            result.goal.urgency = interrupt.urgency;
            result.goal.target_key = interrupt.threat_key;
            result.source_kind = WorldAgentGoalSourceKind::ExternalInterrupt;
            result.reason_keys.push_back("interrupt." + std::to_string(static_cast<int>(interrupt.kind)));
            return Result<SelectionResult>::ok(result);
        }
    }
    return Result<SelectionResult>::ok(result);
}

Result<WorldAgentGoalSelector::SelectionResult> WorldAgentGoalSelector::selectFromActiveContext(
    const ExecutionContext& exec_ctx) {
    SelectionResult result;
    if (!hasActiveGoal(exec_ctx)) {
        return Result<SelectionResult>::ok(result);
    }
    // Reconstruct goal from active frame
    for (const auto& frame : exec_ctx.goal_stack) {
        if (frame.frame_id == exec_ctx.active_frame_id.value()) {
            result.has_goal = true;
            result.goal.goal_id = frame.goal_id;
            result.goal.actor_key = frame.actor_key;
            result.source_kind = WorldAgentGoalSourceKind::QueuedGoal;
            result.reason_keys.push_back("resume_active_goal");
            return Result<SelectionResult>::ok(result);
        }
    }
    return Result<SelectionResult>::ok(result);
}

Result<WorldAgentGoalSelector::SelectionResult> WorldAgentGoalSelector::selectFromQueuedGoals(
    const WorldAgentTickRequest& request) {
    SelectionResult result;
    if (!request.queued_goal_ids.empty()) {
        result.has_goal = true;
        result.goal.goal_id = request.queued_goal_ids[0];
        result.goal.actor_key = request.actor_key;
        result.source_kind = WorldAgentGoalSourceKind::QueuedGoal;
        result.reason_keys.push_back("queued_goal");
    }
    return Result<SelectionResult>::ok(result);
}

Result<WorldAgentGoalSelector::SelectionResult> WorldAgentGoalSelector::selectFromNeeds(
    const WorldAgentDecisionContext& context) {
    SelectionResult result;
    // MVP minimal fake: if no goal and resources visible, create AcquireObject goal
    if (!context.visible_resources.empty()) {
        result.has_goal = true;
        result.goal.goal_id = "need.acquire." + context.visible_resources[0].resource_key;
        result.goal.actor_key = context.actor_key;
        result.goal.kind = AgentGoalKind::AcquireObject;
        result.goal.target_key = context.visible_resources[0].resource_key;
        result.source_kind = WorldAgentGoalSourceKind::InternalNeed;
        result.reason_keys.push_back("internal_need_acquire");
    }
    return Result<SelectionResult>::ok(result);
}

Result<WorldAgentGoalSelector::SelectionResult> WorldAgentGoalSelector::select(
    const WorldAgentDecisionContext& context,
    const WorldAgentTickRequest& request) {

    // Priority 1: External interrupts
    auto interrupt_result = selectFromInterrupts(context);
    if (interrupt_result.is_ok() && interrupt_result.value().has_goal) {
        return interrupt_result;
    }

    // Priority 2: Active execution context
    if (context.existing_execution_context.has_value()) {
        auto active_result = selectFromActiveContext(context.existing_execution_context.value());
        if (active_result.is_ok() && active_result.value().has_goal) {
            return active_result;
        }
    }

    // Priority 3: Queued goals
    auto queued_result = selectFromQueuedGoals(request);
    if (queued_result.is_ok() && queued_result.value().has_goal) {
        return queued_result;
    }

    // Priority 4: Internal needs (minimal MVP)
    auto needs_result = selectFromNeeds(context);
    if (needs_result.is_ok() && needs_result.value().has_goal) {
        return needs_result;
    }

    // No goal
    return Result<SelectionResult>::ok(SelectionResult{});
}

} // namespace pathfinder::world_agent_execution
