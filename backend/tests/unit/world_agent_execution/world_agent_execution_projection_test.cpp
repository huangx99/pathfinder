#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/world_agent_projection_bridge.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::agent_reasoning;
using namespace pathfinder::goal_execution;
using namespace pathfinder::foundation;

void run_world_agent_execution_projection_tests() {
    std::cout << "Running world_agent_execution projection tests:" << std::endl;

    WorldAgentProjectionBridge bridge;

    WorldAgentTickResult tick_result;
    tick_result.ok = true;
    tick_result.decision = WorldAgentExecutionDecision::IssuedCommand;
    tick_result.failure_kind = WorldAgentExecutionFailureKind::None;
    tick_result.actor_key = "npc_001";

    AgentGoal goal;
    goal.goal_id = "g1";
    tick_result.selected_goal = goal;

    AgentPlanStep step;
    step.action_key = "gather";
    tick_result.selected_step = step;

    pathfinder::world_command::WorldCommandDto cmd;
    cmd.command_kind = pathfinder::world_command::WorldCommandKind::Gather;
    tick_result.issued_command = cmd;

    InternalBlocker blocker;
    blocker.safe_summary_zh_cn = "safe_blocker";
    tick_result.blockers.push_back(blocker);

    tick_result.reason_keys.push_back("reason_1");

    auto proj = bridge.build(tick_result);
    assert(proj.actor_key == "npc_001");
    assert(proj.decision == "issued_command");
    assert(proj.failure_kind == "none");
    assert(proj.current_goal_summary == "g1");
    assert(proj.current_step_summary == "gather");
    assert(proj.issued_command_kind == "gather");
    assert(proj.blocker_summaries.size() == 1);
    assert(proj.blocker_summaries[0] == "safe_blocker");
    assert(proj.trace_keys.size() == 1);

    // No hidden truth leaked
    assert(proj.current_goal_summary.find("hidden_truth") == std::string::npos);

    std::cout << "All projection tests passed!" << std::endl;
}
