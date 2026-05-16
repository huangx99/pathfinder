#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
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

void run_p7_agent_runtime_unknown_fruit_flow() {
    // Step 1: Create initial state
    auto state = UnknownFruitFixture::createInitialState();
    assert(state.validateBasic().is_ok());

    // Step 2: Configure runtime
    AgentRuntimeOptions opts;
    opts.include_cognition_claims = true;
    opts.include_explore_candidates = true;
    opts.submit_to_pipeline = true;
    opts.submit_action_allowlist.push_back(ActionId(std::string("eat")));

    // Step 3: Build tick request
    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_player_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.command_source = CommandSource::Ai;
    req.options = opts;

    // Step 4: Execute runtime tick
    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    auto& r = result.value();

    // Step 5: Verify pipeline succeeded
    assert(r.status == AgentRuntimeStatus::PipelineSucceeded);
    assert(r.trace.pipeline_submitted);
    assert(r.trace.command_created);
    std::cout << "  PASS: p7_pipeline_succeeded\n";

    // Step 6: Verify P3 semantics (hunger 80 -> 60)
    auto* actor = state.actor_store.findActor(EntityId(std::string("actor_001")));
    assert(actor != nullptr);
    assert(actor->hunger == 60);
    assert(actor->health == 100);
    std::cout << "  PASS: p7_hunger_semantics\n";

    // Step 7: Verify object consumed
    auto* obj = state.object_store.findObject(ObjectId(std::string("obj_unknown_fruit_001")));
    assert(obj != nullptr);
    assert(obj->flag == ObjectRuntimeFlag::Consumed);
    std::cout << "  PASS: p7_object_consumed\n";

    // Step 8: Verify cognition claim
    assert(state.cognition_state.claims().size() == 1);
    auto* claim = state.cognition_state.findEdibleClaim(
        EntityId(std::string("actor_001")),
        ObjectDefinitionId(std::string("unknown_fruit")),
        ActionId(std::string("eat")));
    assert(claim != nullptr);
    assert(claim->confidence > 0.0);
    assert(claim->evidence_count == 1);
    std::cout << "  PASS: p7_cognition_claim\n";

    // Step 9: Verify trace has all phases
    bool has_obs = false, has_as = false, has_dec = false, has_cmd = false, has_pipe = false;
    for (const auto& key : r.trace.phase_keys) {
        if (key == "observation_built") has_obs = true;
        if (key == "action_space_built") has_as = true;
        if (key == "decision_made") has_dec = true;
        if (key == "command_created") has_cmd = true;
        if (key == "pipeline_executed") has_pipe = true;
    }
    assert(has_obs && has_as && has_dec && has_cmd && has_pipe);
    std::cout << "  PASS: p7_trace_phases\n";

    // Step 10: Second tick with consumed object - should not produce eat submission
    AgentTickRequest req2;
    req2.binding = req.binding;
    req2.state = &state;
    req2.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    req2.issued_tick = Tick(1);
    req2.random_seed = RandomSeed(43);
    req2.command_source = CommandSource::Ai;
    req2.options = opts;

    auto result2 = runtime.tickOne(req2);
    assert(result2.is_ok());
    // Consumed object is skipped by observation, so no eat candidate
    assert(result2.value().status == AgentRuntimeStatus::NoDecision);
    std::cout << "  PASS: p7_consumed_no_resubmit\n";

    std::cout << "P7 unknown fruit runtime flow tests passed!\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P7 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "p7_agent_runtime_unknown_fruit_flow") {
        run_p7_agent_runtime_unknown_fruit_flow();
        return 0;
    } else if (test_name == "p7_agent_runtime_threat_no_submit") {
        extern void run_p7_agent_runtime_threat_no_submit();
        run_p7_agent_runtime_threat_no_submit();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
