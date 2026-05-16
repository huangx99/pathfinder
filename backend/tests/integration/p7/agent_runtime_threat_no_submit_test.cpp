#include "pathfinder/agent/runtime.h"
#include "pathfinder/agent/policy.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/state/actor_state.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::command;

void run_p7_agent_runtime_threat_no_submit() {
    // Step 1: Create minimal state with actor
    GameState state;
    state.metadata.state_id = GameStateId(std::string("state_threat_test"));
    state.metadata.state_version = StateVersion(1);
    state.metadata.current_tick = Tick(0);

    ActorSurvivalState actor;
    actor.actor_id = EntityId(std::string("actor_001"));
    actor.hunger = 50;
    actor.health = 100;
    state.actor_store.addActor(actor);

    // Step 2: Build observation with fire threat seed
    AgentRuntimeOptions opts;
    opts.submit_to_pipeline = true;
    opts.submit_action_allowlist.push_back(ActionId(std::string("eat")));

    AgentTickRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = &state;
    req.issued_tick = Tick(0);
    req.random_seed = RandomSeed(42);
    req.command_source = CommandSource::Ai;
    req.options = opts;

    ObservedThreatSeed fire_seed;
    fire_seed.source_id = EntityId(std::string("fire_001"));
    fire_seed.threat_type = AgentThreatType::Fire;
    fire_seed.severity = 0.95;
    fire_seed.confidence = 0.99;
    req.visibility.threat_seeds.push_back(fire_seed);

    // Step 3: Execute runtime
    FirstSupportedPolicy policy;
    AgentRuntime runtime(policy);
    auto result = runtime.tickOne(req);
    assert(result.is_ok());
    auto& r = result.value();

    // Step 4: Verify Flee is NOT submitted (not in allowlist)
    assert(r.status == AgentRuntimeStatus::SubmitSkipped);
    assert(!r.trace.pipeline_submitted);
    std::cout << "  PASS: p7_fire_flee_not_submitted\n";

    // Step 5: Verify no state changes
    auto* actor_after = state.actor_store.findActor(EntityId(std::string("actor_001")));
    assert(actor_after != nullptr);
    assert(actor_after->hunger == 50);
    assert(actor_after->health == 100);
    std::cout << "  PASS: p7_no_state_changes\n";

    // Step 6: Verify command was created (flee command_adapter succeeds)
    assert(r.command.has_value());
    std::cout << "  PASS: p7_flee_command_created\n";

    // Step 7: Social threat - CallGroup should not generate command
    GameState state2;
    state2.metadata.state_id = GameStateId(std::string("state_social_test"));
    state2.metadata.state_version = StateVersion(1);
    state2.metadata.current_tick = Tick(0);
    state2.actor_store.addActor(actor);

    AgentTickRequest req2;
    req2.binding = req.binding;
    req2.state = &state2;
    req2.issued_tick = Tick(1);
    req2.random_seed = RandomSeed(43);
    req2.command_source = CommandSource::Ai;
    req2.options = opts;

    ObservedThreatSeed social_seed;
    social_seed.source_id = EntityId(std::string("pack_001"));
    social_seed.threat_type = AgentThreatType::Social;
    social_seed.severity = 0.7;
    social_seed.confidence = 0.8;
    req2.visibility.threat_seeds.push_back(social_seed);

    auto result2 = runtime.tickOne(req2);
    assert(result2.is_ok());
    // CallGroup is command_supported=false, so policy has no supported candidate
    assert(result2.value().status == AgentRuntimeStatus::NoDecision);
    assert(!result2.value().command.has_value());
    std::cout << "  PASS: p7_social_no_decision\n";

    std::cout << "P7 threat no-submit tests passed!\n";
}
