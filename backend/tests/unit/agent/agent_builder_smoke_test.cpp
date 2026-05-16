#include "pathfinder/agent/builder_types.h"
#include <cassert>
#include <iostream>
#include <string>

// Forward declarations
void run_agent_builder_types_tests();
void run_agent_observation_builder_tests();
void run_agent_action_space_builder_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        pathfinder::agent::AgentId id(std::string("agent_builder_001"));
        assert(!id.empty());
        std::cout << "agent builder smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "builder_types") {
        run_agent_builder_types_tests();
        return 0;
    } else if (test_name == "observation_builder") {
        run_agent_observation_builder_tests();
        return 0;
    } else if (test_name == "action_space_builder") {
        run_agent_action_space_builder_tests();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
