#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
#include "pathfinder/agent/runtime_types.h"
#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/object/types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::rules;
using namespace pathfinder::command;
using namespace pathfinder::object;

static AgentTickRequest makeUnknownFruitTickRequest(
    GameState& state, const AgentRuntimeOptions& options) {
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_player_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.command_source = CommandSource::Ai;
    req.options = options;
    return req;
}

void test_build_observation_and_action_space() {
    auto state = UnknownFruitFixture::createInitialState();
    AgentRuntimeOptions opts;
    opts.submit_to_pipeline = false;
    auto req = makeUnknownFruitTickRequest(state, opts);

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    auto& r = result.value();
    assert(r.status == AgentRuntimeStatus::CommandCreated);
    assert(!r.observation.objects.empty());
    assert(!r.action_space.candidates.empty());
    assert(r.trace.phase_keys.size() >= 3);
    std::cout << "  PASS: build_observation_and_action_space\n";
}

void test_missing_object_returns_failed() {
    auto state = UnknownFruitFixture::createInitialState();
    AgentRuntimeOptions opts;
    opts.submit_to_pipeline = false;
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("nonexistent")));
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.options = opts;

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    assert(result.value().status == AgentRuntimeStatus::Failed);
    std::cout << "  PASS: missing_object_returns_failed\n";
}

void test_consumed_object_no_eat_candidate() {
    auto state = UnknownFruitFixture::createInitialState();
    auto* obj = state.object_store.findObject(ObjectId(std::string("obj_unknown_fruit_001")));
    assert(obj != nullptr);
    obj->flag = ObjectRuntimeFlag::Consumed;

    AgentRuntimeOptions opts;
    opts.submit_to_pipeline = false;
    auto req = makeUnknownFruitTickRequest(state, opts);

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    auto& r = result.value();
    // Observation skips consumed, so action space has no eat candidate
    assert(r.action_space.candidates.empty() || r.status == AgentRuntimeStatus::NoDecision);
    std::cout << "  PASS: consumed_object_no_eat_candidate\n";
}

void test_eat_candidate_generates_command() {
    auto state = UnknownFruitFixture::createInitialState();
    AgentRuntimeOptions opts;
    opts.submit_to_pipeline = false;
    auto req = makeUnknownFruitTickRequest(state, opts);

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    auto& r = result.value();
    assert(r.command.has_value());
    assert(r.command->payload.action_id == ActionId(std::string("eat")));
    assert(!r.command->payload.targets.empty());
    std::cout << "  PASS: eat_candidate_generates_command\n";
}

void test_build_command_false_stops_at_decision() {
    auto state = UnknownFruitFixture::createInitialState();
    AgentRuntimeOptions opts;
    opts.build_command = false;
    opts.submit_to_pipeline = false;
    auto req = makeUnknownFruitTickRequest(state, opts);

    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    assert(result.value().status == AgentRuntimeStatus::DecisionMade);
    assert(!result.value().command.has_value());
    std::cout << "  PASS: build_command_false_stops_at_decision\n";
}

void test_explore_not_submitted() {
    auto state = UnknownFruitFixture::createInitialState();
    AgentRuntimeOptions opts;
    opts.include_explore_candidates = true;
    opts.submit_to_pipeline = true;
    opts.submit_action_allowlist.push_back(ActionId(std::string("explore")));
    auto req = makeUnknownFruitTickRequest(state, opts);

    // Create a policy that always selects first candidate (explore is first if unsupported skipped)
    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    // FirstSupportedPolicy skips explore (command_supported=false), selects eat
    // But allowlist only has "explore", so eat should be SubmitSkipped
    assert(result.value().status == AgentRuntimeStatus::SubmitSkipped);
    std::cout << "  PASS: explore_not_submitted\n";
}

void run_agent_runtime_tests() {
    test_build_observation_and_action_space();
    test_missing_object_returns_failed();
    test_consumed_object_no_eat_candidate();
    test_eat_candidate_generates_command();
    test_build_command_false_stops_at_decision();
    test_explore_not_submitted();
    std::cout << "All agent runtime tests passed!\n";
}
