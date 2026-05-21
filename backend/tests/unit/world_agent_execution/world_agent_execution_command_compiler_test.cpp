#include "pathfinder/world_agent_execution/world_agent_execution_types.h"
#include "pathfinder/world_agent_execution/agent_plan_command_compiler.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_agent_execution;
using namespace pathfinder::agent_reasoning;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;
using namespace pathfinder::foundation;

void run_world_agent_execution_command_compiler_tests() {
    std::cout << "Running world_agent_execution command compiler tests:" << std::endl;

    AgentPlanCommandCompiler compiler;

    WorldAgentDecisionContext ctx;
    ctx.context_id = "ctx1";
    ctx.actor_key = "npc_001";
    ctx.actor.coord = WorldCellCoord{0, 0, "surface"};
    ctx.tick = 1;

    // Move step
    AgentPlanStep move_step;
    move_step.step_id = "move_1";
    move_step.action_key = "move";
    move_step.target_key = "target_node";

    // Add a visible resource for target resolution
    VisibleResourceRef res;
    res.node_id = "target_node";
    res.resource_key = "neutral_resource";
    res.coord = WorldCellCoord{1, 0, "surface"};
    ctx.visible_resources.push_back(res);

    auto r1 = compiler.compile("npc_001", move_step, ctx, "req_1");
    assert(r1.is_ok());
    assert(r1.value().ok);
    assert(r1.value().step_command_kind == WorldAgentStepCommandKind::Move);
    assert(r1.value().command.command_kind == WorldCommandKind::Move);
    assert(r1.value().command.command_key == "move");
    assert(r1.value().command.target.target_kind == WorldCommandTargetKind::Coordinate);
    assert(r1.value().command.validateBasic().is_ok());
    assert(r1.value().command.source == WorldCommandSource::AgentDecision);
    assert(r1.value().command.actor_key == "npc_001");
    assert(r1.value().command.command_id.find("req_1") != std::string::npos);

    // Gather step adjacent
    AgentPlanStep gather_step;
    gather_step.step_id = "gather_1";
    gather_step.action_key = "gather";
    gather_step.object_key = "neutral_resource";
    gather_step.target_key = "target_node";

    auto r2 = compiler.compile("npc_001", gather_step, ctx, "req_1");
    assert(r2.is_ok());
    assert(r2.value().ok);
    assert(r2.value().step_command_kind == WorldAgentStepCommandKind::Gather);
    assert(r2.value().command.command_kind == WorldCommandKind::Gather);
    assert(r2.value().command.command_key == "gather");
    assert(r2.value().command.target.target_kind == WorldCommandTargetKind::Entity);
    assert(r2.value().command.validateBasic().is_ok());
    assert(r2.value().command.source == WorldCommandSource::AgentDecision);

    // Gather step far away: should emit Move instead
    WorldAgentDecisionContext ctx_far = ctx;
    ctx_far.actor.coord = WorldCellCoord{0, 0, "surface"};
    ctx_far.visible_resources.clear();
    VisibleResourceRef far_res;
    far_res.node_id = "far_node";
    far_res.resource_key = "neutral_resource";
    far_res.coord = WorldCellCoord{5, 5, "surface"};
    ctx_far.visible_resources.push_back(far_res);

    AgentPlanStep far_gather;
    far_gather.step_id = "gather_far";
    far_gather.action_key = "gather";
    far_gather.object_key = "neutral_resource";
    far_gather.target_key = "far_node";

    auto r3 = compiler.compile("npc_001", far_gather, ctx_far, "req_1");
    assert(r3.is_ok());
    // Should compile to Move because not adjacent
    assert(r3.value().step_command_kind == WorldAgentStepCommandKind::Move);
    assert(r3.value().command.command_kind == WorldCommandKind::Move);
    assert(r3.value().command.command_key == "move");
    assert(r3.value().command.validateBasic().is_ok());

    AgentPlanStep unresolved_move;
    unresolved_move.step_id = "move_missing";
    unresolved_move.action_key = "move";
    unresolved_move.target_key = "not_visible";
    auto r_missing = compiler.compile("npc_001", unresolved_move, ctx, "req_1");
    assert(r_missing.is_ok());
    assert(!r_missing.value().ok);

    // Wait step
    AgentPlanStep wait_step;
    wait_step.step_id = "wait_1";
    wait_step.action_key = "wait";

    auto r4 = compiler.compile("npc_001", wait_step, ctx, "req_1");
    assert(r4.is_ok());
    assert(r4.value().ok);
    assert(r4.value().command.command_kind == WorldCommandKind::Wait);
    assert(r4.value().command.command_key == "wait");
    assert(r4.value().command.validateBasic().is_ok());

    std::cout << "All command compiler tests passed!" << std::endl;
}
