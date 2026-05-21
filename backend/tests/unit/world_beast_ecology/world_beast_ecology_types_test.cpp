#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;
using namespace pathfinder::foundation;

void run_world_beast_ecology_enum_roundtrip_tests() {
    std::cout << "Running world_beast_ecology enum roundtrip tests:" << std::endl;

    // BeastTemperamentKind
    assert(toString(BeastTemperamentKind::Unknown) == "unknown");
    assert(toString(BeastTemperamentKind::Predatory) == "predatory");
    assert(toString(BeastTemperamentKind::TestOnly) == "test_only");
    auto t1 = beastTemperamentKindFromString("predatory");
    assert(t1.is_ok() && t1.value() == BeastTemperamentKind::Predatory);
    auto t_bad = beastTemperamentKindFromString("not_real");
    assert(t_bad.is_error());

    // BeastNeedKind
    assert(toString(BeastNeedKind::Hunger) == "hunger");
    assert(toString(BeastNeedKind::Fear) == "fear");
    auto n1 = beastNeedKindFromString("territory");
    assert(n1.is_ok() && n1.value() == BeastNeedKind::Territory);
    auto n_bad = beastNeedKindFromString("invalid");
    assert(n_bad.is_error());

    // BeastPerceptionKind
    assert(toString(BeastPerceptionKind::Effect) == "effect");
    assert(toString(BeastPerceptionKind::Actor) == "actor");
    auto p1 = beastPerceptionKindFromString("sound");
    assert(p1.is_ok() && p1.value() == BeastPerceptionKind::Sound);
    auto p_bad = beastPerceptionKindFromString("invalid");
    assert(p_bad.is_error());

    // BeastActionIntentKind
    assert(toString(BeastActionIntentKind::Flee) == "flee");
    assert(toString(BeastActionIntentKind::Attack) == "attack");
    auto a1 = beastActionIntentKindFromString("approach");
    assert(a1.is_ok() && a1.value() == BeastActionIntentKind::Approach);
    auto a_bad = beastActionIntentKindFromString("invalid");
    assert(a_bad.is_error());

    // BeastDecisionReasonKind
    assert(toString(BeastDecisionReasonKind::LearnedRisk) == "learned_risk");
    assert(toString(BeastDecisionReasonKind::PerceivedDanger) == "perceived_danger");
    auto d1 = beastDecisionReasonKindFromString("perceived_prey");
    assert(d1.is_ok() && d1.value() == BeastDecisionReasonKind::PerceivedPrey);
    auto d_bad = beastDecisionReasonKindFromString("invalid");
    assert(d_bad.is_error());

    // Unknown is invalid
    assert(BeastTemperamentKind::Unknown != BeastTemperamentKind::TestOnly);
    assert(BeastActionIntentKind::Unknown != BeastActionIntentKind::Wait);

    std::cout << "All enum roundtrip tests passed!" << std::endl;
}

void run_world_beast_ecology_profile_validate_tests() {
    std::cout << "Running world_beast_ecology profile validate tests:" << std::endl;

    // Valid profile
    BeastAgentProfile profile;
    profile.profile_key = "test_predator";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.vision_radius = 5;
    profile.base_aggression = 50.0;
    profile.base_fear = 30.0;
    BeastInstinctCapability cap_wait;
    cap_wait.capability_key = "wait";
    cap_wait.action_kind = BeastActionIntentKind::Wait;
    cap_wait.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
    cap_wait.requires_target = false;
    profile.instinct_capabilities.push_back(cap_wait);
    auto v1 = profile.validateBasic();
    assert(v1.is_ok());

    // Missing wait/move capability
    BeastAgentProfile bad_profile;
    bad_profile.profile_key = "bad";
    bad_profile.temperament = BeastTemperamentKind::Predatory;
    bad_profile.vision_radius = 5;
    bad_profile.base_aggression = 50.0;
    bad_profile.base_fear = 30.0;
    BeastInstinctCapability cap_attack;
    cap_attack.capability_key = "attack";
    cap_attack.action_kind = BeastActionIntentKind::Attack;
    cap_attack.command_kind = pathfinder::world_command::WorldCommandKind::Attack;
    cap_attack.requires_target = true;
    bad_profile.instinct_capabilities.push_back(cap_attack);
    auto v2 = bad_profile.validateBasic();
    assert(v2.is_error());

    // Unknown temperament
    BeastAgentProfile bad_temp;
    bad_temp.profile_key = "bad_temp";
    bad_temp.temperament = BeastTemperamentKind::Unknown;
    bad_temp.vision_radius = 5;
    bad_temp.base_aggression = 50.0;
    bad_temp.base_fear = 30.0;
    bad_temp.instinct_capabilities.push_back(cap_wait);
    auto v3 = bad_temp.validateBasic();
    assert(v3.is_error());

    // Out of range vision
    BeastAgentProfile bad_vision;
    bad_vision.profile_key = "bad_vision";
    bad_vision.temperament = BeastTemperamentKind::Passive;
    bad_vision.vision_radius = 100;
    bad_vision.base_aggression = 50.0;
    bad_vision.base_fear = 30.0;
    bad_vision.instinct_capabilities.push_back(cap_wait);
    auto v4 = bad_vision.validateBasic();
    assert(v4.is_error());

    // Valid tick request
    BeastTickRequest req;
    req.request_id = "r1";
    req.actor_key = "beast_1";
    req.tick = 1;
    auto v5 = req.validateBasic();
    assert(v5.is_ok());

    // Invalid tick request
    BeastTickRequest bad_req;
    bad_req.request_id = "";
    bad_req.actor_key = "beast_1";
    auto v6 = bad_req.validateBasic();
    assert(v6.is_error());

    // Valid intent
    BeastActionIntent intent;
    intent.intent_id = "i1";
    intent.actor_key = "beast_1";
    intent.kind = BeastActionIntentKind::Wait;
    intent.command_kind = pathfinder::world_command::WorldCommandKind::Wait;
    intent.reason_kind = BeastDecisionReasonKind::NoValidAction;
    intent.risk_score = 0.0;
    auto v7 = intent.validateBasic();
    assert(v7.is_ok());

    // Intent with missing target
    BeastActionIntent bad_intent;
    bad_intent.intent_id = "i2";
    bad_intent.actor_key = "beast_1";
    bad_intent.kind = BeastActionIntentKind::Attack;
    bad_intent.command_kind = pathfinder::world_command::WorldCommandKind::Attack;
    bad_intent.reason_kind = BeastDecisionReasonKind::PerceivedPrey;
    bad_intent.risk_score = 50.0;
    auto v8 = bad_intent.validateBasic();
    assert(v8.is_error());

    std::cout << "All profile validate tests passed!" << std::endl;
}
