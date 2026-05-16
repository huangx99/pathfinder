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

void run_wolf_pack_action_space_tests() {
    // Step 1: Create a minimal game state for wolf
    GameState state;
    state.metadata.state_id = GameStateId(std::string("state_wolf_test"));
    state.metadata.state_version = StateVersion(1);
    state.metadata.current_tick = Tick(0);

    ActorSurvivalState wolf_actor;
    wolf_actor.actor_id = EntityId(std::string("actor_wolf_001"));
    wolf_actor.hunger = 60;
    wolf_actor.health = 100;
    state.actor_store.addActor(wolf_actor);

    // Step 2: Build observation with social threat seed
    ObservationBuildRequest obs_req;
    obs_req.binding.agent_id = AgentId(std::string("agent_wolf_001"));
    obs_req.binding.primary_actor_id = EntityId(std::string("actor_wolf_001"));
    obs_req.binding.authority = AgentControlAuthority::Primary;
    obs_req.state = &state;

    ObservedThreatSeed social_seed;
    social_seed.source_id = EntityId(std::string("pack_signal_001"));
    social_seed.threat_type = AgentThreatType::Social;
    social_seed.severity = 0.7;
    social_seed.confidence = 0.8;
    obs_req.visibility.threat_seeds.push_back(social_seed);

    ObservationBuilder obs_builder;
    auto obs_result = obs_builder.build(obs_req);
    assert(obs_result.is_ok());
    assert(obs_result.value().observation.threats.size() == 1);
    assert(obs_result.value().observation.threats[0].threat_type == AgentThreatType::Social);
    std::cout << "  PASS: wolf_social_observation_built\n";

    // Step 3: Build action space from observation
    ActionSpaceBuildRequest as_req;
    as_req.observation = obs_result.value().observation;

    ActionSpaceBuilder as_builder;
    auto as_result = as_builder.build(as_req);
    assert(as_result.is_ok());

    // Should have CallGroup candidate with command_supported=false
    bool found_call_group = false;
    for (const auto& c : as_result.value().action_space.candidates) {
        if (c.intent_type == AgentIntentType::CallGroup) {
            found_call_group = true;
            assert(c.command_supported == false);
        }
    }
    assert(found_call_group);
    std::cout << "  PASS: wolf_call_group_candidate_generated\n";

    // Step 4: Verify Adapter rejects CallGroup
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_wolf_001"));
    intent.decision_id = DecisionId(std::string("decision_call_pack_001"));
    intent.actor_id = EntityId(std::string("actor_wolf_001"));
    intent.intent_type = AgentIntentType::CallGroup;
    intent.confidence = 0.7;
    intent.reason_key = "pack_hunting_signal";
    intent.action_id = ActionId(std::string("call_group_pack_signal_001"));
    intent.command_supported_snapshot = false;

    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Ai, Tick(0));
    assert(cmd_result.is_error());
    std::cout << "  PASS: wolf_call_group_adapter_rejected\n";

    std::cout << "All wolf pack action space tests passed!\n";
}
