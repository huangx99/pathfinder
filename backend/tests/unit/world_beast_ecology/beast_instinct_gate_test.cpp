#include "pathfinder/world_beast_ecology/beast_instinct_gate.h"
#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;

void run_world_beast_ecology_instinct_gate_tests() {
    std::cout << "Running world_beast_ecology instinct gate tests:" << std::endl;

    BeastInstinctGate gate;

    BeastAgentProfile profile;
    profile.profile_key = "test";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.base_aggression = 50.0;
    profile.base_fear = 30.0;

    BeastInstinctCapability cap_wait;
    cap_wait.capability_key = "wait";
    cap_wait.action_kind = BeastActionIntentKind::Wait;
    cap_wait.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
    cap_wait.requires_target = false;
    cap_wait.risk_limit = 100.0;
    profile.instinct_capabilities.push_back(cap_wait);

    BeastInstinctCapability cap_move;
    cap_move.capability_key = "move";
    cap_move.action_kind = BeastActionIntentKind::Wander;
    cap_move.command_kind = pathfinder::world_command::WorldCommandKind::Move;
    cap_move.requires_target = false;
    cap_move.risk_limit = 50.0;
    profile.instinct_capabilities.push_back(cap_move);

    // Wait should be allowed
    BeastActionIntent wait_intent;
    wait_intent.kind = BeastActionIntentKind::Wait;
    wait_intent.risk_score = 0.0;
    auto r1 = gate.check(profile, wait_intent);
    assert(r1.allowed == true);
    assert(r1.blocker_key.empty());

    // Wander with risk within limit
    BeastActionIntent wander_intent;
    wander_intent.kind = BeastActionIntentKind::Wander;
    wander_intent.risk_score = 30.0;
    auto r2 = gate.check(profile, wander_intent);
    assert(r2.allowed == true);

    // Wander with risk exceeding limit
    BeastActionIntent risky_intent;
    risky_intent.kind = BeastActionIntentKind::Wander;
    risky_intent.risk_score = 80.0;
    auto r3 = gate.check(profile, risky_intent);
    assert(r3.allowed == false);
    assert(r3.blocker_key == "instinct_risk_limit_exceeded");

    // Attack not in capabilities
    BeastActionIntent attack_intent;
    attack_intent.kind = BeastActionIntentKind::Attack;
    attack_intent.risk_score = 50.0;
    auto r4 = gate.check(profile, attack_intent);
    assert(r4.allowed == false);
    assert(r4.blocker_key == "missing_instinct_capability");

    // Add attack capability
    BeastInstinctCapability cap_attack;
    cap_attack.capability_key = "attack";
    cap_attack.action_kind = BeastActionIntentKind::Attack;
    cap_attack.command_kind = pathfinder::world_command::WorldCommandKind::Attack;
    cap_attack.requires_target = true;
    cap_attack.risk_limit = 70.0;
    profile.instinct_capabilities.push_back(cap_attack);

    auto r5 = gate.check(profile, attack_intent);
    assert(r5.allowed == true);

    std::cout << "All instinct gate tests passed!" << std::endl;
}
