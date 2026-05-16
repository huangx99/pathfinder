#include "pathfinder/agent/types.h"
#include <cassert>
#include <iostream>
#include <string>

// Forward declarations of test functions from other test files
void run_agent_types_tests();
void run_agent_definition_tests();
void run_agent_binding_tests();
void run_agent_observation_tests();
void run_agent_action_space_tests();
void run_agent_intent_tests();
void run_agent_command_adapter_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        // Basic smoke test for agent module
        pathfinder::agent::AgentId id(std::string("agent_001"));
        assert(!id.empty());
        assert(id.value() == "agent_001");

        pathfinder::agent::AgentDefinitionId def_id(std::string("def_spider"));
        assert(!def_id.empty());
        assert(def_id.value() == "def_spider");

        std::cout << "agent smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_agent_types_tests();
        return 0;
    } else if (test_name == "definition") {
        run_agent_definition_tests();
        return 0;
    } else if (test_name == "binding") {
        run_agent_binding_tests();
        return 0;
    } else if (test_name == "observation") {
        run_agent_observation_tests();
        return 0;
    } else if (test_name == "action_space") {
        run_agent_action_space_tests();
        return 0;
    } else if (test_name == "intent") {
        run_agent_intent_tests();
        return 0;
    } else if (test_name == "command_adapter") {
        run_agent_command_adapter_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
