#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/state/game_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::command;

void test_status_to_string() {
    assert(toString(AgentRuntimeStatus::Unknown) == "Unknown");
    assert(toString(AgentRuntimeStatus::ObservationBuilt) == "ObservationBuilt");
    assert(toString(AgentRuntimeStatus::ActionSpaceBuilt) == "ActionSpaceBuilt");
    assert(toString(AgentRuntimeStatus::DecisionMade) == "DecisionMade");
    assert(toString(AgentRuntimeStatus::CommandCreated) == "CommandCreated");
    assert(toString(AgentRuntimeStatus::SubmitSkipped) == "SubmitSkipped");
    assert(toString(AgentRuntimeStatus::PipelineSucceeded) == "PipelineSucceeded");
    assert(toString(AgentRuntimeStatus::PipelineFailed) == "PipelineFailed");
    assert(toString(AgentRuntimeStatus::NoDecision) == "NoDecision");
    assert(toString(AgentRuntimeStatus::Failed) == "Failed");
    std::cout << "  PASS: status_to_string\n";
}

void test_status_from_string() {
    auto r = agentRuntimeStatusFromString("PipelineSucceeded");
    assert(r.is_ok());
    assert(r.value() == AgentRuntimeStatus::PipelineSucceeded);

    auto r2 = agentRuntimeStatusFromString("InvalidStatus");
    assert(r2.is_error());
    std::cout << "  PASS: status_from_string\n";
}

void test_tick_request_null_state_fails() {
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = nullptr;
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    assert(req.validateBasic().is_error());
    std::cout << "  PASS: tick_request_null_state_fails\n";
}

void test_tick_request_unknown_source_fails() {
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    // Need a non-null state, use a stack GameState
    pathfinder::state::GameState state;
    state.metadata.state_id = pathfinder::foundation::GameStateId(std::string("test_state"));
    req.state = &state;
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.command_source = CommandSource::Unknown;
    assert(req.validateBasic().is_error());
    std::cout << "  PASS: tick_request_unknown_source_fails\n";
}

void test_tick_request_empty_allowlist_fails() {
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    pathfinder::state::GameState state;
    state.metadata.state_id = pathfinder::foundation::GameStateId(std::string("test_state"));
    req.state = &state;
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.options.submit_to_pipeline = true;
    req.options.submit_action_allowlist.clear();
    assert(req.validateBasic().is_error());
    std::cout << "  PASS: tick_request_empty_allowlist_fails\n";
}

void test_tick_result_validate_basic() {
    AgentTickResult result;
    result.status = AgentRuntimeStatus::PipelineSucceeded;
    assert(result.validateBasic().is_ok());
    std::cout << "  PASS: tick_result_validate_basic\n";
}

void run_agent_runtime_types_tests() {
    test_status_to_string();
    test_status_from_string();
    test_tick_request_null_state_fails();
    test_tick_request_unknown_source_fails();
    test_tick_request_empty_allowlist_fails();
    test_tick_result_validate_basic();
    std::cout << "All agent runtime types tests passed!\n";
}
