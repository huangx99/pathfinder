#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/agent/action_space_builder.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/state/game_state.h"
#include "pathfinder/state/actor_state.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;
using namespace pathfinder::state;
using namespace pathfinder::command;

void run_spider_fire_action_space_tests() {
    // Step 1: Create a minimal game state for spider
    GameState state;
    state.metadata.state_id = GameStateId(std::string("state_spider_test"));
    state.metadata.state_version = StateVersion(1);
    state.metadata.current_tick = Tick(0);

    // Add spider actor
    ActorSurvivalState spider_actor;
    spider_actor.actor_id = EntityId(std::string("actor_spider_001"));
    spider_actor.hunger = 50;
    spider_actor.health = 100;
    state.actor_store.addActor(spider_actor);

    // Step 2: Build observation with fire threat seed
    ObservationBuildRequest obs_req;
    obs_req.binding.agent_id = AgentId(std::string("agent_spider_001"));
    obs_req.binding.primary_actor_id = EntityId(std::string("actor_spider_001"));
    obs_req.binding.authority = AgentControlAuthority::Primary;
    obs_req.state = &state;

    ObservedThreatSeed fire_seed;
    fire_seed.source_id = EntityId(std::string("fire_001"));
    fire_seed.threat_type = AgentThreatType::Fire;
    fire_seed.severity = 0.95;
    fire_seed.confidence = 0.99;
    obs_req.visibility.threat_seeds.push_back(fire_seed);

    ObservationBuilder obs_builder;
    auto obs_result = obs_builder.build(obs_req);
    assert(obs_result.is_ok());
    assert(obs_result.value().observation.threats.size() == 1);
    assert(obs_result.value().observation.threats[0].threat_type == AgentThreatType::Fire);
    std::cout << "  PASS: spider_fire_observation_built\n";

    // Step 3: Build action space from observation
    ActionSpaceBuildRequest as_req;
    as_req.observation = obs_result.value().observation;

    ActionSpaceBuilder as_builder;
    auto as_result = as_builder.build(as_req);
    assert(as_result.is_ok());

    // Should have Flee candidate
    bool found_flee = false;
    const AgentActionCandidate* flee_candidate = nullptr;
    for (const auto& c : as_result.value().action_space.candidates) {
        if (c.intent_type == AgentIntentType::Flee) {
            found_flee = true;
            flee_candidate = &c;
            assert(c.command_supported == true);
            assert(c.required_target_types.size() == 1);
            assert(c.required_target_types[0] == ActionTargetType::Entity);
        }
    }
    assert(found_flee);
    std::cout << "  PASS: spider_fire_flee_candidate_generated\n";

    // Step 4: Verify Flee can be converted to AvoidRisk CommandEnvelope
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_spider_001"));
    intent.decision_id = DecisionId(std::string("decision_flee_001"));
    intent.actor_id = EntityId(std::string("actor_spider_001"));
    intent.intent_type = AgentIntentType::Flee;
    intent.confidence = 0.99;
    intent.reason_key = "flee_from_fire";
    intent.action_id = flee_candidate->action_id;
    intent.command_supported_snapshot = flee_candidate->command_supported;

    ActionTarget target;
    target.target_type = ActionTargetType::Entity;
    target.target_id = TargetId(std::string("fire_001"));
    intent.targets.push_back(target);

    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Ai, Tick(0));
    assert(cmd_result.is_ok());
    assert(cmd_result.value().payload.intent == CommandIntent::AvoidRisk);
    std::cout << "  PASS: spider_flee_maps_to_avoid_risk\n";

    // Step 5: Do NOT submit to RulePipeline (P6 boundary)
    std::cout << "All spider fire action space tests passed!\n";
}
