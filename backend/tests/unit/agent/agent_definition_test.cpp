#include "pathfinder/agent/definition.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;

// Test fixtures
AgentDefinition createSpiderDefinition() {
    AgentDefinition def;
    def.definition_id = AgentDefinitionId(std::string("agent_def_spider_micro"));
    def.display_name_key = "agent_spider";
    def.scale = AgentScale::MicroCreature;
    def.cognition_band = AgentCognitionBand::Reflex;
    def.embodiment = AgentEmbodiment::SingleBody;
    def.default_controller = AgentControllerType::Ai;
    def.profile.caution = 0.7;
    def.profile.aggression = 0.3;
    return def;
}

AgentDefinition createWolfDefinition() {
    AgentDefinition def;
    def.definition_id = AgentDefinitionId(std::string("agent_def_wolf_small"));
    def.display_name_key = "agent_wolf";
    def.scale = AgentScale::SmallCreature;
    def.cognition_band = AgentCognitionBand::Instinct;
    def.embodiment = AgentEmbodiment::SingleBody;
    def.default_controller = AgentControllerType::Ai;
    def.profile.aggression = 0.7;
    def.profile.cooperation = 0.5;
    def.profile.sociality = 0.6;
    return def;
}

AgentDefinition createPioneerDefinition() {
    AgentDefinition def;
    def.definition_id = AgentDefinitionId(std::string("agent_def_pioneer_player"));
    def.display_name_key = "agent_pioneer";
    def.scale = AgentScale::Individual;
    def.cognition_band = AgentCognitionBand::ToolUse;
    def.embodiment = AgentEmbodiment::SingleBody;
    def.default_controller = AgentControllerType::Player;
    def.profile.curiosity = 0.8;
    def.profile.caution = 0.4;
    def.profile.cooperation = 0.7;
    return def;
}

AgentDefinition createAlienCivilizationDefinition() {
    AgentDefinition def;
    def.definition_id = AgentDefinitionId(std::string("agent_def_alien_civilization"));
    def.display_name_key = "agent_alien_civ";
    def.scale = AgentScale::Interstellar;
    def.cognition_band = AgentCognitionBand::Civilizational;
    def.embodiment = AgentEmbodiment::DistributedGroup;
    def.default_controller = AgentControllerType::Ai;
    def.profile.curiosity = 0.9;
    def.profile.caution = 0.8;
    return def;
}

void test_spider_definition_valid() {
    auto def = createSpiderDefinition();
    auto result = def.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: spider_definition_valid\n";
}

void test_wolf_definition_valid() {
    auto def = createWolfDefinition();
    auto result = def.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: wolf_definition_valid\n";
}

void test_pioneer_definition_valid() {
    auto def = createPioneerDefinition();
    auto result = def.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: pioneer_definition_valid\n";
}

void test_alien_civilization_definition_valid() {
    auto def = createAlienCivilizationDefinition();
    auto result = def.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: alien_civilization_definition_valid\n";
}

void test_definition_missing_id() {
    auto def = createPioneerDefinition();
    def.definition_id = AgentDefinitionId();
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: definition_missing_id\n";
}

void test_definition_scale_unknown() {
    auto def = createPioneerDefinition();
    def.scale = AgentScale::Unknown;
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: definition_scale_unknown\n";
}

void test_definition_cognition_band_unknown() {
    auto def = createPioneerDefinition();
    def.cognition_band = AgentCognitionBand::Unknown;
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: definition_cognition_band_unknown\n";
}

void test_definition_embodiment_unknown() {
    auto def = createPioneerDefinition();
    def.embodiment = AgentEmbodiment::Unknown;
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: definition_embodiment_unknown\n";
}

void test_profile_value_below_zero() {
    auto def = createPioneerDefinition();
    def.profile.curiosity = -0.1;
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: profile_value_below_zero\n";
}

void test_profile_value_above_one() {
    auto def = createPioneerDefinition();
    def.profile.caution = 1.5;
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: profile_value_above_one\n";
}

void test_definition_bad_id_format() {
    auto def = createPioneerDefinition();
    def.definition_id = AgentDefinitionId(std::string("bad id with space"));
    auto result = def.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: definition_bad_id_format\n";
}

void run_agent_definition_tests() {
    test_spider_definition_valid();
    test_wolf_definition_valid();
    test_pioneer_definition_valid();
    test_alien_civilization_definition_valid();
    test_definition_missing_id();
    test_definition_scale_unknown();
    test_definition_cognition_band_unknown();
    test_definition_embodiment_unknown();
    test_profile_value_below_zero();
    test_profile_value_above_one();
    test_definition_bad_id_format();
    std::cout << "All agent definition tests passed!\n";
}
