#include <cassert>
#include <iostream>
#include <string>

// Forward declarations of test functions
void run_command_types_tests();
void run_action_target_tests();
void run_action_command_tests();
void run_command_envelope_tests();
void run_command_validation_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, target, action_command, envelope, validation" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "command smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_command_types_tests();
        return 0;
    } else if (test_name == "target") {
        run_action_target_tests();
        return 0;
    } else if (test_name == "action_command") {
        run_action_command_tests();
        return 0;
    } else if (test_name == "envelope") {
        run_command_envelope_tests();
        return 0;
    } else if (test_name == "validation") {
        run_command_validation_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
