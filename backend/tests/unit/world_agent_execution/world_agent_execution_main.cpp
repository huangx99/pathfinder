#include <cassert>
#include <iostream>
#include <string>

void run_world_agent_execution_enum_roundtrip_tests();
void run_world_agent_execution_context_store_tests();
void run_world_agent_execution_context_builder_tests();
void run_world_agent_execution_knowledge_guard_tests();
void run_world_agent_execution_command_compiler_tests();
void run_world_agent_execution_coordinator_tests();
void run_world_agent_execution_projection_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available: enum_roundtrip, context_store, context_builder, knowledge_guard, command_compiler, coordinator, projection" << std::endl;
        return 1;
    }
    std::string test_name = argv[1];
    if (test_name == "enum_roundtrip") {
        run_world_agent_execution_enum_roundtrip_tests();
    } else if (test_name == "context_store") {
        run_world_agent_execution_context_store_tests();
    } else if (test_name == "context_builder") {
        run_world_agent_execution_context_builder_tests();
    } else if (test_name == "knowledge_guard") {
        run_world_agent_execution_knowledge_guard_tests();
    } else if (test_name == "command_compiler") {
        run_world_agent_execution_command_compiler_tests();
    } else if (test_name == "coordinator") {
        run_world_agent_execution_coordinator_tests();
    } else if (test_name == "projection") {
        run_world_agent_execution_projection_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
