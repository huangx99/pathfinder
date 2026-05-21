#include <cassert>
#include <iostream>
#include <string>

void run_world_beast_ecology_enum_roundtrip_tests();
void run_world_beast_ecology_profile_validate_tests();
void run_world_beast_ecology_perception_tests();
void run_world_beast_ecology_instinct_gate_tests();
void run_world_beast_ecology_policy_tests();
void run_world_beast_ecology_compiler_tests();
void run_world_beast_ecology_coordinator_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available: enum_roundtrip, profile_validate, perception, instinct_gate, policy, compiler, coordinator" << std::endl;
        return 1;
    }
    std::string test_name = argv[1];
    if (test_name == "enum_roundtrip") {
        run_world_beast_ecology_enum_roundtrip_tests();
    } else if (test_name == "profile_validate") {
        run_world_beast_ecology_profile_validate_tests();
    } else if (test_name == "perception") {
        run_world_beast_ecology_perception_tests();
    } else if (test_name == "instinct_gate") {
        run_world_beast_ecology_instinct_gate_tests();
    } else if (test_name == "policy") {
        run_world_beast_ecology_policy_tests();
    } else if (test_name == "compiler") {
        run_world_beast_ecology_compiler_tests();
    } else if (test_name == "coordinator") {
        run_world_beast_ecology_coordinator_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
