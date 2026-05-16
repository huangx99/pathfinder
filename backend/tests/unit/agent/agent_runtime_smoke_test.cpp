#include <cassert>
#include <iostream>
#include <string>

void run_agent_policy_tests();
void run_agent_runtime_types_tests();
void run_agent_runtime_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "agent runtime smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "policy") {
        run_agent_policy_tests();
        return 0;
    } else if (test_name == "runtime_types") {
        run_agent_runtime_types_tests();
        return 0;
    } else if (test_name == "runtime") {
        run_agent_runtime_tests();
        return 0;
    }

    std::cout << "Unknown test: " << test_name << std::endl;
    return 1;
}
