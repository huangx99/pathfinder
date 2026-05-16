#include "pathfinder/agent/definition.h"
#include "pathfinder/agent/binding.h"
#include "pathfinder/agent/observation.h"
#include "pathfinder/agent/action_space.h"
#include "pathfinder/agent/intent.h"
#include "pathfinder/agent/command_adapter.h"
#include "pathfinder/command/types.h"
#include <iostream>
#include <cassert>

using namespace pathfinder::agent;
using namespace pathfinder::command;
using namespace pathfinder::foundation;

void run_spider_flee_fire_tests() {
    // Step 1: Create spider AgentDefinition (MicroCreature + Reflex + SingleBody)
    AgentDefinition spider_def;
    spider_def.definition_id = AgentDefinitionId(std::string("agent_def_spider"));
    spider_def.display_name_key = "agent_spider";
    spider_def.scale = AgentScale::MicroCreature;
    spider_def.cognition_band = AgentCognitionBand::Reflex;
    spider_def.embodiment = AgentEmbodiment::SingleBody;
    spider_def.default_controller = AgentControllerType::Ai;
    spider_def.profile.curiosity = 0.2;
    spider_def.profile.caution = 0.9;
    spider_def.profile.aggression = 0.1;
    spider_def.profile.sociality = 0.1;
    spider_def.profile.cooperation = 0.1;
    spider_def.profile.learning_rate_hint = 0.0;
    assert(spider_def.validateBasic().is_ok());
    std::cout << "  PASS: spider_definition_valid\n";

    // Step 2: Create fire_danger observation
    AgentObservation observation;
    observation.agent_id = AgentId(std::string("agent_spider_001"));
    observation.observer_actor_id = EntityId("actor_spider_001");
    observation.state_version = StateVersion(1);
    observation.observed_tick = Tick(0);

    AgentObservedThreat fire_threat;
    fire_threat.source_id = EntityId("fire_source_001");
    fire_threat.threat_type = AgentThreatType::Fire;
    fire_threat.severity = 0.95;
    fire_threat.confidence = 0.99;
    observation.threats.push_back(fire_threat);
    assert(observation.validateBasic().is_ok());
    std::cout << "  PASS: fire_danger_observation_valid\n";

    // Step 3: Create action space with Flee/AvoidRisk candidate
    AgentActionSpace action_space;
    action_space.agent_id = AgentId(std::string("agent_spider_001"));
    action_space.state_version = StateVersion(1);

    AgentActionCandidate flee_candidate;
    flee_candidate.action_id = ActionId("flee");
    flee_candidate.intent_type = AgentIntentType::Flee;
    flee_candidate.command_supported = true;
    flee_candidate.reason_key = "flee_from_fire";
    action_space.candidates.push_back(flee_candidate);
    assert(action_space.validateBasic().is_ok());
    std::cout << "  PASS: flee_action_candidate_valid\n";

    // Step 4: Create Flee AgentIntent
    AgentIntent intent;
    intent.agent_id = AgentId(std::string("agent_spider_001"));
    intent.decision_id = DecisionId(std::string("decision_flee_001"));
    intent.actor_id = EntityId("actor_spider_001");
    intent.intent_type = AgentIntentType::Flee;
    intent.confidence = 0.99;
    intent.reason_key = "fire_danger_escape";
    intent.action_id = ActionId("flee");

    ActionTarget flee_target;
    flee_target.target_type = ActionTargetType::Entity;
    flee_target.target_id = TargetId("fire_source_001");
    intent.targets.push_back(flee_target);
    assert(intent.validateBasic().is_ok());
    std::cout << "  PASS: flee_intent_valid\n";

    // Step 5: Convert to CommandEnvelope (Flee maps to AvoidRisk)
    AgentCommandAdapter adapter;
    auto cmd_result = adapter.toCommandEnvelope(intent, CommandSource::Ai, Tick(0));
    assert(cmd_result.is_ok());

    auto& cmd = cmd_result.value();
    assert(cmd.validateBasic().is_ok());
    assert(cmd.source == CommandSource::Ai);
    assert(cmd.payload.intent == CommandIntent::AvoidRisk);
    assert(cmd.payload.actor_id == EntityId("actor_spider_001"));
    std::cout << "  PASS: flee_maps_to_avoid_risk_envelope\n";

    // Step 6: Verify no flee resolver exists (boundary check)
    // P5 does NOT submit to RulePipeline - flee resolver does not exist
    // This test only proves Agent expression capability

    std::cout << "All spider flee fire tests passed!\n";
}
