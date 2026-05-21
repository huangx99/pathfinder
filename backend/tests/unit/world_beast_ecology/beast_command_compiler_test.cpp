#include "pathfinder/world_beast_ecology/beast_command_compiler.h"
#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;

void run_world_beast_ecology_compiler_tests() {
    std::cout << "Running world_beast_ecology compiler tests:" << std::endl;

    BeastCommandCompiler compiler;

    // Wait intent
    BeastActionIntent wait_intent;
    wait_intent.intent_id = "i_wait";
    wait_intent.actor_key = "beast_1";
    wait_intent.kind = BeastActionIntentKind::Wait;
    wait_intent.command_kind = WorldCommandKind::Wait;
    wait_intent.reason_kind = BeastDecisionReasonKind::NoValidAction;
    wait_intent.risk_score = 0.0;

    auto r1 = compiler.compile(wait_intent, "req_1", "beast_1");
    assert(r1.is_ok());
    auto cmd1 = r1.value();
    assert(cmd1.source == WorldCommandSource::BeastDecision);
    assert(cmd1.command_key == "wait");
    assert(cmd1.command_id == "req_1_i_wait");
    assert(cmd1.actor_key == "beast_1");
    assert(cmd1.target.target_kind == WorldCommandTargetKind::None);

    // Approach intent
    BeastActionIntent approach_intent;
    approach_intent.intent_id = "i_approach";
    approach_intent.actor_key = "beast_1";
    approach_intent.kind = BeastActionIntentKind::Approach;
    approach_intent.target_ref = "prey_actor_1";
    approach_intent.target_key = "prey_entity";
    approach_intent.target_coord = WorldCellCoord{2, 0, "surface"};
    approach_intent.command_kind = WorldCommandKind::Move;
    approach_intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
    approach_intent.risk_score = 30.0;

    auto r2 = compiler.compile(approach_intent, "req_2", "beast_1");
    assert(r2.is_ok());
    auto cmd2 = r2.value();
    assert(cmd2.source == WorldCommandSource::BeastDecision);
    assert(cmd2.command_key == "move");
    assert(cmd2.target.target_kind == WorldCommandTargetKind::Coordinate);
    assert(cmd2.target.target_coord.has_value());
    assert(cmd2.target.target_coord.value().x == 2);

    // Attack intent
    BeastActionIntent attack_intent;
    attack_intent.intent_id = "i_attack";
    attack_intent.actor_key = "beast_1";
    attack_intent.kind = BeastActionIntentKind::Attack;
    attack_intent.target_ref = "prey_actor_1";
    attack_intent.command_kind = WorldCommandKind::Attack;
    attack_intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
    attack_intent.risk_score = 60.0;

    auto r3 = compiler.compile(attack_intent, "req_3", "beast_1");
    assert(r3.is_ok());
    auto cmd3 = r3.value();
    assert(cmd3.source == WorldCommandSource::BeastDecision);
    assert(cmd3.command_key == "attack");
    assert(cmd3.target.target_kind == WorldCommandTargetKind::Actor);
    assert(cmd3.target.target_actor_key == "prey_actor_1");

    // Flee intent
    BeastActionIntent flee_intent;
    flee_intent.intent_id = "i_flee";
    flee_intent.actor_key = "beast_1";
    flee_intent.kind = BeastActionIntentKind::Flee;
    flee_intent.target_ref = "danger_source_1";
    flee_intent.target_coord = WorldCellCoord{-1, 0, "surface"};
    flee_intent.command_kind = WorldCommandKind::Flee;
    flee_intent.reason_kind = BeastDecisionReasonKind::PerceivedDanger;
    flee_intent.risk_score = 80.0;

    auto r4 = compiler.compile(flee_intent, "req_4", "beast_1");
    assert(r4.is_ok());
    auto cmd4 = r4.value();
    assert(cmd4.source == WorldCommandSource::BeastDecision);
    assert(cmd4.command_key == "flee");

    // Wander without target_coord must fall back to Wait (no hardcoded origin)
    BeastActionIntent wander_intent;
    wander_intent.intent_id = "i_wander";
    wander_intent.actor_key = "beast_1";
    wander_intent.kind = BeastActionIntentKind::Wander;
    wander_intent.command_kind = WorldCommandKind::Move;
    wander_intent.reason_kind = BeastDecisionReasonKind::NoValidAction;
    wander_intent.risk_score = 0.0;

    auto r5 = compiler.compile(wander_intent, "req_5", "beast_1");
    assert(r5.is_ok());
    auto cmd5 = r5.value();
    assert(cmd5.source == WorldCommandSource::BeastDecision);
    assert(cmd5.command_kind == WorldCommandKind::Wait);
    assert(cmd5.command_key == "wait");
    assert(cmd5.target.target_kind == WorldCommandTargetKind::None);

    std::cout << "All compiler tests passed!" << std::endl;
}
