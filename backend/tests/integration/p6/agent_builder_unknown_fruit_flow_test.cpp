#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/agent/observation_builder.h"
#include "pathfinder/agent/action_space_builder.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/object/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::agent;
using namespace pathfinder::pipeline;
using namespace pathfinder::state;
using namespace pathfinder::foundation;
using namespace pathfinder::command;
using namespace pathfinder::object;

void run_p6_agent_builder_unknown_fruit_flow() {
    // Step 1: Create initial state using P3 fixture
    auto state = UnknownFruitFixture::createInitialState();
    assert(state.validateBasic().is_ok());

    // Step 2: Build observation using ObservationBuilder
    ObservationBuildRequest obs_req;
    obs_req.binding.agent_id = AgentId(std::string("agent_player_001"));
    obs_req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    obs_req.binding.authority = AgentControlAuthority::Primary;
    obs_req.state = &state;
    obs_req.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));

    ObservationBuilder obs_builder;
    auto obs_result = obs_builder.build(obs_req);
    assert(obs_result.is_ok());

    auto& observation = obs_result.value().observation;
    assert(observation.objects.size() == 1);
    assert(observation.objects[0].object_id == ObjectId(std::string("obj_unknown_fruit_001")));
    assert(observation.objects[0].known_edible_confidence == 0.0);
    std::cout << "  PASS: p6_observation_built\n";

    // Step 3: Build action space using ActionSpaceBuilder
    ActionSpaceBuildRequest as_req;
    as_req.observation = observation;

    ActionSpaceBuilder as_builder;
    auto as_result = as_builder.build(as_req);
    assert(as_result.is_ok());

    auto& action_space = as_result.value().action_space;
    // Should have Eat + Explore candidates
    assert(action_space.candidates.size() >= 1);

    // Find the Eat candidate
    const AgentActionCandidate* eat_candidate = nullptr;
    for (const auto& c : action_space.candidates) {
        if (c.intent_type == AgentIntentType::Eat) {
            eat_candidate = &c;
            break;
        }
    }
    assert(eat_candidate != nullptr);
    assert(eat_candidate->command_supported == true);
    std::cout << "  PASS: p6_action_space_built\n";

    // Step 4: Select Eat candidate and construct AgentIntent from candidate data
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_player_001"));
    intent.decision_id = DecisionId(std::string("decision_eat_001"));
    intent.actor_id = EntityId(std::string("actor_001"));
    intent.intent_type = AgentIntentType::Eat;
    intent.confidence = 0.8;
    intent.reason_key = eat_candidate->reason_key;
    intent.action_id = eat_candidate->command_action_id.empty()
        ? eat_candidate->action_id : eat_candidate->command_action_id;
    intent.command_supported_snapshot = eat_candidate->command_supported;
    // Copy targets from candidate.suggested_targets instead of hand-writing
    intent.targets = eat_candidate->suggested_targets;
    assert(intent.validateBasic().is_ok());
    std::cout << "  PASS: p6_intent_constructed\n";

    // Step 5: Convert to CommandEnvelope using adapter
    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Player, Tick(0));
    assert(cmd_result.is_ok());
    auto& cmd = cmd_result.value();
    assert(cmd.validateBasic().is_ok());
    assert(cmd.payload.intent == CommandIntent::Experiment);
    std::cout << "  PASS: p6_command_envelope_created\n";

    // Step 6: Submit to RulePipeline
    PipelineContext context;
    context.command = cmd;
    context.state_metadata = state.metadata;
    context.game_state = &state;
    context.random_seed = RandomSeed(42);

    RulePipeline pipeline;
    auto result = pipeline.execute(context);
    assert(result.is_ok());

    auto& pipeline_result = result.value();
    assert(pipeline_result.status == PipelineStatus::Succeeded);
    assert(!pipeline_result.state_changes.empty());
    std::cout << "  PASS: p6_pipeline_succeeded\n";

    // Step 7: Verify P3 semantic results
    auto* actor = state.actor_store.findActor(EntityId(std::string("actor_001")));
    assert(actor != nullptr);
    assert(actor->hunger == 60);
    assert(actor->health == 100);

    auto* obj = state.object_store.findObject(ObjectId(std::string("obj_unknown_fruit_001")));
    assert(obj != nullptr);
    assert(obj->flag == ObjectRuntimeFlag::Consumed);

    assert(state.cognition_state.claims().size() == 1);
    std::cout << "  PASS: p6_p3_semantics_stable\n";

    // Step 8: Build observation again - consumed object should be skipped
    ObservationBuildRequest obs_req2;
    obs_req2.binding.agent_id = AgentId(std::string("agent_player_001"));
    obs_req2.binding.primary_actor_id = EntityId(std::string("actor_001"));
    obs_req2.binding.authority = AgentControlAuthority::Primary;
    obs_req2.state = &state;
    obs_req2.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));

    ObservationBuilder obs_builder2;
    auto obs_result2 = obs_builder2.build(obs_req2);
    assert(obs_result2.is_ok());
    assert(obs_result2.value().observation.objects.empty());
    assert(obs_result2.value().trace.skipped_object_ids.size() == 1);
    std::cout << "  PASS: p6_consumed_object_skipped_after_eat\n";

    // Step 9: Build observation with cognition claims - should reflect learned knowledge
    ObservationBuildRequest obs_req3;
    obs_req3.binding.agent_id = AgentId(std::string("agent_player_001"));
    obs_req3.binding.primary_actor_id = EntityId(std::string("actor_001"));
    obs_req3.binding.authority = AgentControlAuthority::Primary;
    obs_req3.state = &state;
    obs_req3.visibility.visible_object_ids.push_back(ObjectId(std::string("obj_unknown_fruit_001")));
    obs_req3.include_cognition_claims = true;

    // Verify the cognition claim exists using the semantic query API
    auto* claim = state.cognition_state.findEdibleClaim(
        EntityId(std::string("actor_001")),
        ObjectDefinitionId(std::string("unknown_fruit")),
        ActionId(std::string("eat")));
    assert(claim != nullptr);
    assert(claim->confidence > 0.0);
    assert(claim->evidence_count == 1);
    std::cout << "  PASS: p6_cognition_claim_stable\n";

    std::cout << "P6 unknown fruit builder flow tests passed\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P6 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "agent_builder_unknown_fruit_flow") {
        run_p6_agent_builder_unknown_fruit_flow();
        std::cout << "P6 agent builder unknown fruit flow tests passed" << std::endl;
        return 0;
    } else if (test_name == "spider_fire_action_space") {
        // Forward declaration - defined in spider_fire_action_space_test.cpp
        extern void run_spider_fire_action_space_tests();
        run_spider_fire_action_space_tests();
        return 0;
    } else if (test_name == "wolf_pack_action_space") {
        extern void run_wolf_pack_action_space_tests();
        run_wolf_pack_action_space_tests();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
