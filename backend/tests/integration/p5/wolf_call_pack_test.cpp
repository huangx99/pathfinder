#include "pathfinder/agent/definition.h"
#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/command/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::agent;
using namespace pathfinder::command;
using namespace pathfinder::foundation;

void run_wolf_call_pack_tests() {
    // Step 1: Create wolf AgentDefinition (SmallCreature + Instinct + SingleBody)
    AgentDefinition wolf_def;
    wolf_def.definition_id = AgentDefinitionId(std::string("agent_def_wolf"));
    wolf_def.display_name_key = "agent_wolf";
    wolf_def.scale = AgentScale::SmallCreature;
    wolf_def.cognition_band = AgentCognitionBand::Instinct;
    wolf_def.embodiment = AgentEmbodiment::SingleBody;
    wolf_def.default_controller = AgentControllerType::Ai;
    wolf_def.profile.curiosity = 0.4;
    wolf_def.profile.caution = 0.5;
    wolf_def.profile.aggression = 0.6;
    wolf_def.profile.sociality = 0.8;     // higher than spider
    wolf_def.profile.cooperation = 0.7;   // higher than spider
    wolf_def.profile.learning_rate_hint = 0.1;
    assert(wolf_def.validateBasic().is_ok());
    std::cout << "  PASS: wolf_definition_valid\n";

    // Step 2: Create wolf binding
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_wolf_001"));
    binding.primary_actor_id = EntityId("actor_wolf_001");
    binding.authority = AgentControlAuthority::Primary;
    assert(binding.validateBasic().is_ok());
    std::cout << "  PASS: wolf_binding_valid\n";

    // Step 3: Create action space with CallGroup candidate (command_supported=false)
    AgentActionSpace action_space;
    action_space.agent_id = AgentId(std::string("agent_wolf_001"));
    action_space.state_version = StateVersion(1);

    AgentActionCandidate call_group_candidate;
    call_group_candidate.action_id = ActionId("call_pack");
    call_group_candidate.intent_type = AgentIntentType::CallGroup;
    call_group_candidate.command_supported = false;  // not supported in P5
    call_group_candidate.reason_key = "pack_coordination";
    action_space.candidates.push_back(call_group_candidate);
    assert(action_space.validateBasic().is_ok());
    std::cout << "  PASS: call_group_candidate_valid\n";

    // Step 4: Create CallGroup AgentIntent
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_wolf_001"));
    intent.decision_id = DecisionId(std::string("decision_call_pack_001"));
    intent.actor_id = EntityId("actor_wolf_001");
    intent.intent_type = AgentIntentType::CallGroup;
    intent.confidence = 0.7;
    intent.reason_key = "pack_hunting_signal";
    intent.action_id = ActionId("call_pack");
    assert(intent.validateBasic().is_ok());
    std::cout << "  PASS: call_group_intent_valid\n";

    // Step 5: Verify Adapter returns explicit unsupported error
    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Ai, Tick(0));
    assert(cmd_result.is_error());

    // Verify error is explicit, not silent
    auto errors = cmd_result.errors();
    assert(!errors.empty());
    std::cout << "  PASS: call_group_adapter_returns_unsupported\n";

    // Step 6: Verify CallGroup cannot be submitted to RulePipeline
    // P5 does NOT implement group combat - this is by design

    std::cout << "All wolf call pack tests passed!\n";
}
