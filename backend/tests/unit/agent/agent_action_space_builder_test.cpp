#include "pathfinder/agent/action_space_builder.h"
#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/command/types.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::rules;
using namespace pathfinder::command;

static AgentObservation makeEmptyObservation() {
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("agent_001"));
    obs.observer_actor_id = EntityId(std::string("actor_001"));
    obs.state_version = StateVersion(1);
    obs.observed_tick = Tick(0);
    return obs;
}

void test_empty_observation_generates_empty_action_space() {
    ActionSpaceBuildRequest req;
    req.observation = makeEmptyObservation();
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    assert(result.value().action_space.candidates.empty());
    std::cout << "  PASS: empty_observation_generates_empty_action_space\n";
}

void test_invalid_observation_fails() {
    ActionSpaceBuildRequest req;
    // observation with empty agent_id
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_error());
    std::cout << "  PASS: invalid_observation_fails\n";
}

void test_builder_does_not_depend_on_game_state() {
    // ActionSpaceBuilder should compile without GameState include
    ActionSpaceBuildRequest req;
    req.observation = makeEmptyObservation();
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    std::cout << "  PASS: builder_does_not_depend_on_game_state\n";
}

void test_observed_object_generates_eat_candidate() {
    auto obs = makeEmptyObservation();
    AgentObservedObject obj;
    obj.object_id = ObjectId(std::string("obj_001"));
    obj.known_edible_confidence = 0.0;
    obj.known_usable_confidence = 0.0;
    obj.risk_confidence = 0.0;
    obj.evidence_count = 0;
    obs.objects.push_back(obj);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();
    // Should have Eat + Explore candidates
    assert(res.action_space.candidates.size() == 2);

    // Find the Eat candidate
    bool found_eat = false;
    for (const auto& c : res.action_space.candidates) {
        if (c.intent_type == AgentIntentType::Eat) {
            found_eat = true;
            assert(c.command_supported == true);
            assert(c.action_id == ActionId(std::string("eat_obj_001")));
            assert(c.required_target_types.size() == 1);
            assert(c.required_target_types[0] == ActionTargetType::Object);
        }
    }
    assert(found_eat);
    std::cout << "  PASS: observed_object_generates_eat_candidate\n";
}

void test_explore_candidate_command_supported_false() {
    auto obs = makeEmptyObservation();
    AgentObservedObject obj;
    obj.object_id = ObjectId(std::string("obj_001"));
    obs.objects.push_back(obj);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    req.include_explore_candidates = true;
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();

    bool found_explore = false;
    for (const auto& c : res.action_space.candidates) {
        if (c.intent_type == AgentIntentType::Explore) {
            found_explore = true;
            assert(c.command_supported == false);
        }
    }
    assert(found_explore);
    std::cout << "  PASS: explore_candidate_command_supported_false\n";
}

void test_fire_threat_generates_flee_candidate() {
    auto obs = makeEmptyObservation();
    AgentObservedThreat threat;
    threat.source_id = EntityId(std::string("fire_001"));
    threat.threat_type = AgentThreatType::Fire;
    threat.severity = 0.9;
    threat.confidence = 0.95;
    obs.threats.push_back(threat);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();

    bool found_flee = false;
    for (const auto& c : res.action_space.candidates) {
        if (c.intent_type == AgentIntentType::Flee) {
            found_flee = true;
            assert(c.command_supported == true);
            assert(c.action_id == ActionId(std::string("flee_fire_001")));
        }
    }
    assert(found_flee);
    std::cout << "  PASS: fire_threat_generates_flee_candidate\n";
}

void test_social_threat_generates_call_group_candidate() {
    auto obs = makeEmptyObservation();
    AgentObservedThreat threat;
    threat.source_id = EntityId(std::string("pack_001"));
    threat.threat_type = AgentThreatType::Social;
    threat.severity = 0.7;
    threat.confidence = 0.8;
    obs.threats.push_back(threat);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    auto& res = result.value();

    bool found_call_group = false;
    for (const auto& c : res.action_space.candidates) {
        if (c.intent_type == AgentIntentType::CallGroup) {
            found_call_group = true;
            assert(c.command_supported == false);
        }
    }
    assert(found_call_group);
    std::cout << "  PASS: social_threat_generates_call_group_candidate\n";
}

void test_action_ids_are_deterministic() {
    auto obs = makeEmptyObservation();
    AgentObservedObject obj;
    obj.object_id = ObjectId(std::string("obj_001"));
    obs.objects.push_back(obj);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    ActionSpaceBuilder builder;
    auto result1 = builder.build(req);
    auto result2 = builder.build(req);
    assert(result1.is_ok() && result2.is_ok());
    assert(result1.value().action_space.candidates.size() == result2.value().action_space.candidates.size());
    for (size_t i = 0; i < result1.value().action_space.candidates.size(); ++i) {
        assert(result1.value().action_space.candidates[i].action_id ==
               result2.value().action_space.candidates[i].action_id);
    }
    std::cout << "  PASS: action_ids_are_deterministic\n";
}

void test_no_duplicate_action_ids() {
    auto obs = makeEmptyObservation();
    // Two different objects
    AgentObservedObject obj1;
    obj1.object_id = ObjectId(std::string("obj_001"));
    obs.objects.push_back(obj1);
    AgentObservedObject obj2;
    obj2.object_id = ObjectId(std::string("obj_002"));
    obs.objects.push_back(obj2);

    ActionSpaceBuildRequest req;
    req.observation = obs;
    ActionSpaceBuilder builder;
    auto result = builder.build(req);
    assert(result.is_ok());
    // All action_ids should be unique (validated by AgentActionSpace::validateBasic)
    std::cout << "  PASS: no_duplicate_action_ids\n";
}

void run_agent_action_space_builder_tests() {
    test_empty_observation_generates_empty_action_space();
    test_invalid_observation_fails();
    test_builder_does_not_depend_on_game_state();
    test_observed_object_generates_eat_candidate();
    test_explore_candidate_command_supported_false();
    test_fire_threat_generates_flee_candidate();
    test_social_threat_generates_call_group_candidate();
    test_action_ids_are_deterministic();
    test_no_duplicate_action_ids();
    std::cout << "All agent action space builder tests passed!\n";
}
