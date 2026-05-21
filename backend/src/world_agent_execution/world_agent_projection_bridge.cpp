#include "pathfinder/world_agent_execution/world_agent_projection_bridge.h"

namespace pathfinder::world_agent_execution {

WorldAgentProjectionBridge::WorldAgentProjectionBridge() {
}

WorldAgentProjectionBridge::AgentProjection WorldAgentProjectionBridge::build(const WorldAgentTickResult& tick_result) {
    AgentProjection proj;
    proj.actor_key = tick_result.actor_key;
    proj.decision = toString(tick_result.decision);
    proj.failure_kind = toString(tick_result.failure_kind);

    if (tick_result.selected_goal.has_value()) {
        proj.current_goal_summary = tick_result.selected_goal.value().goal_id;
    }
    if (tick_result.selected_step.has_value()) {
        proj.current_step_summary = tick_result.selected_step.value().action_key;
    }
    if (tick_result.issued_command.has_value()) {
        proj.issued_command_kind = pathfinder::world_command::toString(tick_result.issued_command.value().command_kind);
    }

    for (const auto& blocker : tick_result.blockers) {
        proj.blocker_summaries.push_back(blocker.safe_summary_zh_cn);
    }
    for (const auto& interrupt : tick_result.interrupt_decisions) {
        proj.interrupt_summaries.push_back(interrupt.safe_explanation_zh_cn);
    }

    proj.trace_keys = tick_result.reason_keys;
    return proj;
}

} // namespace pathfinder::world_agent_execution
