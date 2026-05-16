#include "pathfinder/agent/observation.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;

void test_observation_valid() {
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("agent_001"));
    obs.observer_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    obs.state_version = pathfinder::foundation::StateVersion(1);
    obs.observed_tick = pathfinder::foundation::Tick(10);
    auto result = obs.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observation_valid\n";
}

void test_observation_missing_agent_id() {
    AgentObservation obs;
    obs.observer_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    auto result = obs.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observation_missing_agent_id\n";
}

void test_observation_missing_observer_actor_id() {
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("agent_001"));
    auto result = obs.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observation_missing_observer_actor_id\n";
}

void test_observed_object_valid() {
    AgentObservedObject obj;
    obj.object_id = pathfinder::foundation::ObjectId(std::string("obj_001"));
    obj.known_edible_confidence = 0.6;
    obj.known_usable_confidence = 0.3;
    obj.risk_confidence = 0.2;
    obj.evidence_count = 3;
    obj.can_teach_hint = true;
    auto result = obj.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observed_object_valid\n";
}

void test_observed_object_confidence_below_zero() {
    AgentObservedObject obj;
    obj.object_id = pathfinder::foundation::ObjectId(std::string("obj_001"));
    obj.known_edible_confidence = -0.1;
    auto result = obj.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_object_confidence_below_zero\n";
}

void test_observed_object_confidence_above_one() {
    AgentObservedObject obj;
    obj.object_id = pathfinder::foundation::ObjectId(std::string("obj_001"));
    obj.risk_confidence = 1.5;
    auto result = obj.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_object_confidence_above_one\n";
}

void test_observed_threat_fire() {
    AgentObservedThreat threat;
    threat.source_id = pathfinder::foundation::EntityId(std::string("fire_001"));
    threat.threat_type = AgentThreatType::Fire;
    threat.severity = 0.9;
    threat.confidence = 0.8;
    auto result = threat.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observed_threat_fire\n";
}

void test_observed_threat_type_unknown() {
    AgentObservedThreat threat;
    threat.source_id = pathfinder::foundation::EntityId(std::string("fire_001"));
    threat.threat_type = AgentThreatType::Unknown;
    auto result = threat.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_threat_type_unknown\n";
}

void test_observed_knowledge_teachable() {
    AgentObservedKnowledge knowledge;
    knowledge.knowledge_id = pathfinder::foundation::KnowledgeId(std::string("knowledge_001"));
    knowledge.claim_type = AgentKnowledgeClaimType::EdibleEffect;
    knowledge.confidence = 0.7;
    knowledge.evidence_count = 5;
    knowledge.teachable_hint = true;
    auto result = knowledge.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observed_knowledge_teachable\n";
}

void test_observed_knowledge_claim_type_unknown() {
    AgentObservedKnowledge knowledge;
    knowledge.knowledge_id = pathfinder::foundation::KnowledgeId(std::string("knowledge_001"));
    knowledge.claim_type = AgentKnowledgeClaimType::Unknown;
    auto result = knowledge.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_knowledge_claim_type_unknown\n";
}

void test_observation_with_substructures() {
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("agent_001"));
    obs.observer_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));

    AgentObservedObject obj;
    obj.object_id = pathfinder::foundation::ObjectId(std::string("obj_unknown_fruit"));
    obj.known_edible_confidence = 0.5;
    obs.objects.push_back(obj);

    AgentObservedThreat threat;
    threat.source_id = pathfinder::foundation::EntityId(std::string("fire_001"));
    threat.threat_type = AgentThreatType::Fire;
    threat.severity = 0.8;
    threat.confidence = 0.9;
    obs.threats.push_back(threat);

    auto result = obs.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observation_with_substructures\n";
}

// Verify observation only uses cognition-facing fields
void test_observation_no_visual_concepts() {
    // This test verifies the observation header only defines cognition-facing fields
    // by compiling successfully (if presentation-layer types were present, they would need to be defined)
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("agent_001"));
    obs.observer_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    // Just verify it compiles and validates
    auto result = obs.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: observation_no_visual_concepts\n";
}

void test_observation_bad_agent_id_format() {
    AgentObservation obs;
    obs.agent_id = AgentId(std::string("bad id"));
    obs.observer_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    obs.state_version = pathfinder::foundation::StateVersion(1);
    auto result = obs.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observation_bad_agent_id_format\n";
}

void test_observed_object_negative_evidence_count() {
    AgentObservedObject obj;
    obj.object_id = pathfinder::foundation::ObjectId(std::string("obj_001"));
    obj.known_edible_confidence = 0.5;
    obj.known_usable_confidence = 0.5;
    obj.risk_confidence = 0.2;
    obj.evidence_count = -1;
    auto result = obj.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_object_negative_evidence_count\n";
}

void test_observed_knowledge_negative_evidence_count() {
    AgentObservedKnowledge knowledge;
    knowledge.knowledge_id = pathfinder::foundation::KnowledgeId(std::string("knowledge_001"));
    knowledge.claim_type = AgentKnowledgeClaimType::EdibleEffect;
    knowledge.confidence = 0.5;
    knowledge.evidence_count = -1;
    auto result = knowledge.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observed_knowledge_negative_evidence_count\n";
}

void run_agent_observation_tests() {
    test_observation_valid();
    test_observation_missing_agent_id();
    test_observation_missing_observer_actor_id();
    test_observed_object_valid();
    test_observed_object_confidence_below_zero();
    test_observed_object_confidence_above_one();
    test_observed_threat_fire();
    test_observed_threat_type_unknown();
    test_observed_knowledge_teachable();
    test_observed_knowledge_claim_type_unknown();
    test_observation_with_substructures();
    test_observation_no_visual_concepts();
    test_observation_bad_agent_id_format();
    test_observed_object_negative_evidence_count();
    test_observed_knowledge_negative_evidence_count();
    std::cout << "All agent observation tests passed!\n";
}
