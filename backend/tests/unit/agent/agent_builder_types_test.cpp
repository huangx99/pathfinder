#include "pathfinder/agent/builder_types.h"
#include "pathfinder/agent/binding.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;
using namespace pathfinder::foundation;

void test_visibility_input_valid() {
    VisibilityInput vis;
    vis.visible_object_ids.push_back(ObjectId(std::string("obj_001")));
    vis.visible_object_ids.push_back(ObjectId(std::string("obj_002")));
    auto result = vis.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: visibility_input_valid\n";
}

void test_visibility_input_empty_valid() {
    VisibilityInput vis;
    auto result = vis.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: visibility_input_empty_valid\n";
}

void test_visibility_input_duplicate_object_id() {
    VisibilityInput vis;
    vis.visible_object_ids.push_back(ObjectId(std::string("obj_001")));
    vis.visible_object_ids.push_back(ObjectId(std::string("obj_001")));
    auto result = vis.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: visibility_input_duplicate_object_id\n";
}

void test_visibility_input_bad_object_id_format() {
    VisibilityInput vis;
    vis.visible_object_ids.push_back(ObjectId(std::string("bad id")));
    auto result = vis.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: visibility_input_bad_object_id_format\n";
}

void test_threat_seed_valid() {
    ObservedThreatSeed seed;
    seed.source_id = EntityId(std::string("fire_001"));
    seed.threat_type = AgentThreatType::Fire;
    seed.severity = 0.9;
    seed.confidence = 0.95;
    auto result = seed.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: threat_seed_valid\n";
}

void test_threat_seed_missing_source_id() {
    ObservedThreatSeed seed;
    seed.threat_type = AgentThreatType::Fire;
    seed.severity = 0.9;
    seed.confidence = 0.95;
    auto result = seed.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: threat_seed_missing_source_id\n";
}

void test_threat_seed_unknown_type() {
    ObservedThreatSeed seed;
    seed.source_id = EntityId(std::string("fire_001"));
    seed.threat_type = AgentThreatType::Unknown;
    seed.severity = 0.9;
    seed.confidence = 0.95;
    auto result = seed.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: threat_seed_unknown_type\n";
}

void test_threat_seed_severity_out_of_range() {
    ObservedThreatSeed seed;
    seed.source_id = EntityId(std::string("fire_001"));
    seed.threat_type = AgentThreatType::Fire;
    seed.severity = 1.5;
    seed.confidence = 0.95;
    auto result = seed.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: threat_seed_severity_out_of_range\n";
}

void test_threat_seed_confidence_out_of_range() {
    ObservedThreatSeed seed;
    seed.source_id = EntityId(std::string("fire_001"));
    seed.threat_type = AgentThreatType::Fire;
    seed.severity = 0.9;
    seed.confidence = -0.1;
    auto result = seed.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: threat_seed_confidence_out_of_range\n";
}

void test_observation_build_result_validates_observation() {
    ObservationBuildResult result;
    result.observation.agent_id = AgentId(std::string("agent_001"));
    result.observation.observer_actor_id = EntityId(std::string("actor_001"));
    result.observation.state_version = StateVersion(1);
    result.observation.observed_tick = Tick(0);
    auto r = result.validateBasic();
    assert(r.is_ok());
    std::cout << "  PASS: observation_build_result_validates_observation\n";
}

void test_action_space_build_result_validates_action_space() {
    ActionSpaceBuildResult result;
    result.action_space.agent_id = AgentId(std::string("agent_001"));
    result.action_space.state_version = StateVersion(1);
    auto r = result.validateBasic();
    assert(r.is_ok());
    std::cout << "  PASS: action_space_build_result_validates_action_space\n";
}

void test_observation_build_request_null_state_fails() {
    ObservationBuildRequest req;
    req.binding.agent_id = AgentId(std::string("agent_001"));
    req.binding.primary_actor_id = EntityId(std::string("actor_001"));
    req.binding.authority = AgentControlAuthority::Primary;
    req.state = nullptr;
    auto result = req.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: observation_build_request_null_state_fails\n";
}

void run_agent_builder_types_tests() {
    test_visibility_input_valid();
    test_visibility_input_empty_valid();
    test_visibility_input_duplicate_object_id();
    test_visibility_input_bad_object_id_format();
    test_threat_seed_valid();
    test_threat_seed_missing_source_id();
    test_threat_seed_unknown_type();
    test_threat_seed_severity_out_of_range();
    test_threat_seed_confidence_out_of_range();
    test_observation_build_result_validates_observation();
    test_action_space_build_result_validates_action_space();
    test_observation_build_request_null_state_fails();
    std::cout << "All agent builder types tests passed!\n";
}
