#include "pathfinder/agent/action_space.h"
#include "pathfinder/command/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::command;

void test_empty_action_space_valid() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("agent_001"));
    space.state_version = pathfinder::foundation::StateVersion(1);
    auto result = space.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: empty_action_space_valid\n";
}

void test_action_space_with_eat_candidate() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("agent_001"));
    space.state_version = pathfinder::foundation::StateVersion(1);

    AgentActionCandidate candidate;
    candidate.action_id = pathfinder::foundation::ActionId(std::string("action_eat"));
    candidate.intent_type = AgentIntentType::Eat;
    candidate.required_target_types.push_back(ActionTargetType::Object);
    candidate.command_supported = true;
    candidate.reason_key = "hungry";
    space.candidates.push_back(candidate);

    auto result = space.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: action_space_with_eat_candidate\n";
}

void test_action_space_with_avoid_risk_candidate() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("agent_001"));
    space.state_version = pathfinder::foundation::StateVersion(1);

    AgentActionCandidate candidate;
    candidate.action_id = pathfinder::foundation::ActionId(std::string("action_avoid_risk"));
    candidate.intent_type = AgentIntentType::AvoidRisk;
    candidate.command_supported = true;
    candidate.reason_key = "danger";
    space.candidates.push_back(candidate);

    auto result = space.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: action_space_with_avoid_risk_candidate\n";
}

void test_action_space_duplicate_action_id() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("agent_001"));
    space.state_version = pathfinder::foundation::StateVersion(1);

    AgentActionCandidate c1;
    c1.action_id = pathfinder::foundation::ActionId(std::string("action_eat"));
    c1.intent_type = AgentIntentType::Eat;
    c1.reason_key = "hungry";
    space.candidates.push_back(c1);

    AgentActionCandidate c2;
    c2.action_id = pathfinder::foundation::ActionId(std::string("action_eat"));
    c2.intent_type = AgentIntentType::Eat;
    c2.reason_key = "hungry_again";
    space.candidates.push_back(c2);

    auto result = space.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: action_space_duplicate_action_id\n";
}

void test_action_candidate_unknown_intent_type() {
    AgentActionCandidate candidate;
    candidate.action_id = pathfinder::foundation::ActionId(std::string("action_001"));
    candidate.intent_type = AgentIntentType::Unknown;
    candidate.reason_key = "test";
    auto result = candidate.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: action_candidate_unknown_intent_type\n";
}

void test_action_candidate_target_type_none() {
    AgentActionCandidate candidate;
    candidate.action_id = pathfinder::foundation::ActionId(std::string("action_001"));
    candidate.intent_type = AgentIntentType::Eat;
    candidate.required_target_types.push_back(ActionTargetType::None);
    candidate.reason_key = "test";
    auto result = candidate.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: action_candidate_target_type_none\n";
}

void test_action_space_command_supported_false() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("agent_001"));
    space.state_version = pathfinder::foundation::StateVersion(1);

    AgentActionCandidate candidate;
    candidate.action_id = pathfinder::foundation::ActionId(std::string("action_call_group"));
    candidate.intent_type = AgentIntentType::CallGroup;
    candidate.command_supported = false;
    candidate.reason_key = "pack_signal";
    space.candidates.push_back(candidate);

    auto result = space.validateBasic();
    assert(result.is_ok());
    // command_supported=false means adapter should reject it
    assert(!space.candidates[0].command_supported);
    std::cout << "  PASS: action_space_command_supported_false\n";
}

void test_action_space_bad_agent_id_format() {
    AgentActionSpace space;
    space.agent_id = AgentId(std::string("bad id"));
    space.state_version = pathfinder::foundation::StateVersion(1);
    auto result = space.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: action_space_bad_agent_id_format\n";
}

void run_agent_action_space_tests() {
    test_empty_action_space_valid();
    test_action_space_with_eat_candidate();
    test_action_space_with_avoid_risk_candidate();
    test_action_space_duplicate_action_id();
    test_action_candidate_unknown_intent_type();
    test_action_candidate_target_type_none();
    test_action_space_command_supported_false();
    test_action_space_bad_agent_id_format();
    std::cout << "All agent action space tests passed!\n";
}
