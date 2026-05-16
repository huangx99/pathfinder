#include "pathfinder/agent/binding.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::agent;

void test_single_actor_binding() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: single_actor_binding\n";
}

void test_multi_actor_binding() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.controlled_actor_ids.push_back(pathfinder::foundation::EntityId(std::string("actor_001")));
    binding.controlled_actor_ids.push_back(pathfinder::foundation::EntityId(std::string("actor_002")));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_ok());
    std::cout << "  PASS: multi_actor_binding\n";
}

void test_binding_missing_agent_id() {
    AgentBinding binding;
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_missing_agent_id\n";
}

void test_binding_missing_primary_actor_id() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_missing_primary_actor_id\n";
}

void test_binding_authority_unknown() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_authority_unknown\n";
}

void test_binding_duplicate_actor_ids() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.controlled_actor_ids.push_back(pathfinder::foundation::EntityId(std::string("actor_001")));
    binding.controlled_actor_ids.push_back(pathfinder::foundation::EntityId(std::string("actor_001")));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_duplicate_actor_ids\n";
}

void test_binding_primary_not_in_controlled() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("agent_001"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.controlled_actor_ids.push_back(pathfinder::foundation::EntityId(std::string("actor_002")));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_primary_not_in_controlled\n";
}

void test_binding_bad_agent_id_format() {
    AgentBinding binding;
    binding.agent_id = AgentId(std::string("bad id"));
    binding.primary_actor_id = pathfinder::foundation::EntityId(std::string("actor_001"));
    binding.authority = AgentControlAuthority::Primary;
    auto result = binding.validateBasic();
    assert(result.is_error());
    std::cout << "  PASS: binding_bad_agent_id_format\n";
}

void run_agent_binding_tests() {
    test_single_actor_binding();
    test_multi_actor_binding();
    test_binding_missing_agent_id();
    test_binding_missing_primary_actor_id();
    test_binding_authority_unknown();
    test_binding_duplicate_actor_ids();
    test_binding_primary_not_in_controlled();
    test_binding_bad_agent_id_format();
    std::cout << "All agent binding tests passed!\n";
}
