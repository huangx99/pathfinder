#include "pathfinder/rules/unknown_fruit_fixture.h"
#include "pathfinder/pipeline/rule_pipeline.h"
#include "pathfinder/pipeline/context.h"
#include "pathfinder/pipeline/result.h"
#include "pathfinder/agent/definition.h"
#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/observation.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/event/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::rules;
using namespace pathfinder::pipeline;
using namespace pathfinder::state;
using namespace pathfinder::object;
using namespace pathfinder::cognition;
using namespace pathfinder::foundation;
using namespace pathfinder::event;
using namespace pathfinder::agent;
using namespace pathfinder::command;

// Forward declarations
void run_spider_flee_fire_tests();
void run_wolf_call_pack_tests();

void run_p5_agent_unknown_fruit_flow() {
    // Step 1: Create initial state using P3 fixture
    auto state = UnknownFruitFixture::createInitialState();
    auto state_result = state.validateBasic();
    assert(state_result.is_ok());
    assert(state.cognition_state.claims().empty());

    // Step 2: Create AgentDefinition for pioneer
    AgentDefinition pioneer_def;
    pioneer_def.definition_id = AgentDefinitionId(std::string("agent_def_pioneer_player"));
    pioneer_def.display_name_key = "agent_pioneer";
    pioneer_def.scale = AgentScale::Individual;
    pioneer_def.cognition_band = AgentCognitionBand::ToolUse;
    pioneer_def.embodiment = AgentEmbodiment::SingleBody;
    pioneer_def.default_controller = AgentControllerType::Player;
    pioneer_def.profile.curiosity = 0.8;
    pioneer_def.profile.caution = 0.4;
    assert(pioneer_def.validateBasic().is_ok());

    // Step 3: Create AgentBinding
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_player_001"));
    binding.primary_actor_id = EntityId("actor_001");
    binding.authority = AgentControlAuthority::Primary;
    assert(binding.validateBasic().is_ok());

    // Step 4: Create AgentObservation (what the agent sees)
    AgentObservation observation;
    observation.agent_id = AgentId(std::string("agent_player_001"));
    observation.observer_actor_id = EntityId("actor_001");
    observation.state_version = state.metadata.state_version;
    observation.observed_tick = Tick(0);

    AgentObservedObject observed_fruit;
    observed_fruit.object_id = ObjectId("obj_unknown_fruit_001");
    observed_fruit.known_edible_confidence = 0.5;  // not sure if edible
    observed_fruit.risk_confidence = 0.2;
    observed_fruit.evidence_count = 0;
    observation.objects.push_back(observed_fruit);
    assert(observation.validateBasic().is_ok());

    // Step 5: Create AgentActionSpace
    AgentActionSpace action_space;
    action_space.agent_id = AgentId(std::string("agent_player_001"));
    action_space.state_version = state.metadata.state_version;

    AgentActionCandidate eat_candidate;
    eat_candidate.action_id = ActionId("eat");
    eat_candidate.intent_type = AgentIntentType::Eat;
    eat_candidate.required_target_types.push_back(ActionTargetType::Object);
    eat_candidate.command_supported = true;
    eat_candidate.reason_key = "try_unknown_fruit";
    action_space.candidates.push_back(eat_candidate);
    assert(action_space.validateBasic().is_ok());

    // Step 6: Create AgentIntent (the decision)
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_player_001"));
    intent.decision_id = DecisionId(std::string("decision_eat_001"));
    intent.actor_id = EntityId("actor_001");
    intent.intent_type = AgentIntentType::Eat;
    intent.confidence = 0.8;
    intent.reason_key = "curious_about_fruit";
    intent.action_id = ActionId("eat");  // from ActionSpace candidate

    ActionTarget target;
    target.target_type = ActionTargetType::Object;
    target.target_id = TargetId("obj_unknown_fruit_001");
    intent.targets.push_back(target);
    assert(intent.validateBasic().is_ok());

    // Step 7: Convert AgentIntent to CommandEnvelope using adapter
    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Player, Tick(0));
    assert(cmd_result.is_ok());

    auto& cmd = cmd_result.value();
    assert(cmd.validateBasic().is_ok());
    assert(cmd.source == CommandSource::Player);
    assert(cmd.payload.intent == CommandIntent::Experiment);
    assert(cmd.payload.actor_id == EntityId("actor_001"));

    // Step 8: Submit CommandEnvelope to RulePipeline
    PipelineContext context;
    context.command = cmd;
    context.state_metadata = state.metadata;
    context.game_state = &state;
    context.random_seed = RandomSeed(42);

    RulePipeline pipeline;
    auto result = pipeline.execute(context);
    assert(result.is_ok());

    auto& pipeline_result = result.value();

    // Step 9: Verify pipeline succeeded
    assert(pipeline_result.status == PipelineStatus::Succeeded);
    assert(!pipeline_result.state_changes.empty());
    assert(!pipeline_result.events.empty());

    // Step 10: Verify actor state changed (hunger 80 -> 60)
    auto* actor = state.actor_store.findActor(EntityId("actor_001"));
    assert(actor != nullptr);
    assert(actor->hunger == 60);
    assert(actor->health == 100);

    // Step 11: Verify object consumed
    auto* obj = state.object_store.findObject(ObjectId("obj_unknown_fruit_001"));
    assert(obj != nullptr);
    assert(obj->flag == ObjectRuntimeFlag::Consumed);

    // Step 12: Verify cognition claim was created
    assert(state.cognition_state.claims().size() == 1);

    CognitionKey expected_key;
    expected_key.subject_id = EntityId("actor_001");
    expected_key.object_definition_id = ObjectDefinitionId("unknown_fruit");
    expected_key.action_id = ActionId("eat");
    expected_key.effect_kind = CognitionEffectKind::Edible;

    auto* claim = state.cognition_state.findClaim(expected_key);
    assert(claim != nullptr);
    assert(claim->confidence > 0.0);
    assert(claim->evidence_count == 1);

    std::cout << "  PASS: p5_agent_unknown_fruit_flow\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "P5 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "agent_unknown_fruit_flow") {
        run_p5_agent_unknown_fruit_flow();
        std::cout << "P5 agent unknown fruit flow tests passed" << std::endl;
        return 0;
    } else if (test_name == "spider_flee_fire") {
        run_spider_flee_fire_tests();
        return 0;
    } else if (test_name == "wolf_call_pack") {
        run_wolf_call_pack_tests();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
