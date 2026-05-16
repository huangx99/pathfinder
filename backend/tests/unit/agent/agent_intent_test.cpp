#include "pathfinder/agent/intent.h"
#include "pathfinder/command/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::command;

void test_eat_intent_valid() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Eat;
    intent.confidence = 0.8;
    intent.reason_key = "hungry";

    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = pathfinder::foundation::TargetId(std::string("obj_001"));
    intent.targets.push_back(target);

    auto result = intent.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: eat_intent_valid\n";
}

void test_wait_intent_valid_empty_targets() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Wait;
    intent.confidence = 0.5;
    intent.reason_key = "no_action_needed";

    auto result = intent.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: wait_intent_valid_empty_targets\n";
}

void test_eat_intent_empty_targets_fails() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Eat;
    intent.confidence = 0.8;
    intent.reason_key = "hungry";
    // No targets

    auto result = intent.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: eat_intent_empty_targets_fails\n";
}

void test_intent_confidence_out_of_range() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Wait;
    intent.confidence = 1.5;
    intent.reason_key = "test";

    auto result = intent.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: intent_confidence_out_of_range\n";
}

void test_intent_empty_reason_key_fails() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Wait;
    intent.confidence = 0.5;
    // No reason_key

    auto result = intent.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: intent_empty_reason_key_fails\n";
}

void test_decision_valid() {
    AgentDecision decision;
    decision.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    decision.agent_id = AgentId(std::string("agent_001"));
    decision.observation_state_version = pathfinder::foundation::StateVersion(1);
    decision.action_id = pathfinder::foundation::ActionId(std::string("action_eat"));
    decision.reason_key = "hungry";

    decision.selected_intent.agent_id = AgentId(std::string("agent_001"));
    decision.selected_intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    decision.selected_intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    decision.selected_intent.intent_type = AgentIntentType::Eat;
    decision.selected_intent.confidence = 0.8;
    decision.selected_intent.reason_key = "hungry";

    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = pathfinder::foundation::TargetId(std::string("obj_001"));
    decision.selected_intent.targets.push_back(target);

    auto result = decision.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: decision_valid\n";
}

void test_decision_missing_decision_id() {
    AgentDecision decision;
    decision.agent_id = AgentId(std::string("agent_001"));
    decision.action_id = pathfinder::foundation::ActionId(std::string("action_eat"));
    decision.reason_key = "hungry";

    auto result = decision.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: decision_missing_decision_id\n";
}

void test_intent_bad_agent_id_format() {
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("bad id"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_001"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Wait;
    intent.confidence = 0.5;
    intent.reason_key = "test";
    auto result = intent.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: intent_bad_agent_id_format\n";
}

void run_agent_intent_tests() {
    test_eat_intent_valid();
    test_wait_intent_valid_empty_targets();
    test_eat_intent_empty_targets_fails();
    test_intent_confidence_out_of_range();
    test_intent_empty_reason_key_fails();
    test_decision_valid();
    test_decision_missing_decision_id();
    test_intent_bad_agent_id_format();
    std::cout << "All agent intent tests passed!\n";
}
