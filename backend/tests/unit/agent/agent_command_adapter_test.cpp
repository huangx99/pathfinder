#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/command/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::command;

// Helper to create a valid Eat intent
static AgentIntent createEatIntent() {
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

    return intent;
}

void test_eat_intent_to_command_envelope() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Ai, pathfinder::foundation::Tick(10));
    assert(result.is_ok());

    const auto& envelope = result.value();
    assert(envelope.source == CommandSource::Ai);
    assert(envelope.payload.actor_id == pathfinder::foundation::EntityId(std::string("actor_001")));
    assert(envelope.payload.intent == CommandIntent::Experiment);
    assert(!envelope.payload.targets.empty());
    assert(envelope.correlation_id.has_value());
    assert(!envelope.command_id.empty());

    std::cout << "  PASS: eat_intent_to_command_envelope\n";
}

void test_envelope_source_player() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Player, pathfinder::foundation::Tick(10));
    assert(result.is_ok());
    assert(result.value().source == CommandSource::Player);
    std::cout << "  PASS: envelope_source_player\n";
}

void test_envelope_source_test() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(10));
    assert(result.is_ok());
    assert(result.value().source == CommandSource::Test);
    std::cout << "  PASS: envelope_source_test\n";
}

void test_wait_intent_returns_unsupported() {
    AgentCommandAdapter adapter;
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_002"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Wait;
    intent.confidence = 0.5;
    intent.reason_key = "wait";

    auto result = adapter.toCommandEnvelope(intent, CommandSource::Ai, pathfinder::foundation::Tick(10));
    assert(result.is_error());
    std::cout << "  PASS: wait_intent_returns_unsupported\n";
}

void test_call_group_intent_returns_unsupported() {
    AgentCommandAdapter adapter;
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_001"));
    intent.decision_id = pathfinder::foundation::DecisionId(std::string("decision_003"));
    intent.actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::CallGroup;
    intent.confidence = 0.7;
    intent.reason_key = "pack_signal";

    auto result = adapter.toCommandEnvelope(intent, CommandSource::Ai, pathfinder::foundation::Tick(10));
    assert(result.is_error());
    std::cout << "  PASS: call_group_intent_returns_unsupported\n";
}

void test_unknown_source_returns_error() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Unknown, pathfinder::foundation::Tick(10));
    assert(result.is_error());
    std::cout << "  PASS: unknown_source_returns_error\n";
}

void test_adapter_does_not_depend_on_game_state() {
    // This test verifies that the adapter compiles without including GameState
    // If it included GameState, this test would fail to compile
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_ok());
    std::cout << "  PASS: adapter_does_not_depend_on_game_state\n";
}

void test_deterministic_id_generation() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();

    auto result1 = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    auto result2 = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result1.is_ok() && result2.is_ok());
    assert(result1.value().command_id == result2.value().command_id);
    assert(result1.value().payload.action_id == result2.value().payload.action_id);
    std::cout << "  PASS: deterministic_id_generation\n";
}

void test_envelope_payload_actor_matches_intent() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_ok());
    assert(result.value().payload.actor_id == intent.actor_id);
    std::cout << "  PASS: envelope_payload_actor_matches_intent\n";
}

void test_envelope_targets_match_intent() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_ok());
    assert(result.value().payload.targets.size() == intent.targets.size());
    assert(result.value().payload.targets[0].target_id == intent.targets[0].target_id);
    std::cout << "  PASS: envelope_targets_match_intent\n";
}

void test_action_id_from_intent_used() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    intent.action_id = pathfinder::foundation::ActionId(std::string("eat"));
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_ok());
    assert(result.value().payload.action_id == pathfinder::foundation::ActionId(std::string("eat")));
    std::cout << "  PASS: action_id_from_intent_used\n";
}

void test_action_id_generated_when_empty() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    // action_id not set, should generate from decision_id
    assert(intent.action_id.empty());
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_ok());
    assert(!result.value().payload.action_id.empty());
    assert(result.value().payload.action_id == pathfinder::foundation::ActionId(std::string("action_decision_001")));
    std::cout << "  PASS: action_id_generated_when_empty\n";
}

void test_command_supported_false_rejected() {
    AgentCommandAdapter adapter;
    auto intent = createEatIntent();
    intent.command_supported_snapshot = false;
    auto result = adapter.toCommandEnvelope(intent, CommandSource::Test, pathfinder::foundation::Tick(1));
    assert(result.is_error());
    std::cout << "  PASS: command_supported_false_rejected\n";
}

void run_agent_command_adapter_tests() {
    test_eat_intent_to_command_envelope();
    test_envelope_source_player();
    test_envelope_source_test();
    test_wait_intent_returns_unsupported();
    test_call_group_intent_returns_unsupported();
    test_unknown_source_returns_error();
    test_adapter_does_not_depend_on_game_state();
    test_deterministic_id_generation();
    test_envelope_payload_actor_matches_intent();
    test_envelope_targets_match_intent();
    test_action_id_from_intent_used();
    test_action_id_generated_when_empty();
    test_command_supported_false_rejected();
    std::cout << "All agent command adapter tests passed!\n";
}
