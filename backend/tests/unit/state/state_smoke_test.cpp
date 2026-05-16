#include <iostream>
#include <string>

// Forward declarations of test functions
void run_state_types_tests();
void run_game_state_tests();
void run_state_change_tests();
void run_state_change_gate_tests();
void run_actor_state_tests();
void run_state_value_tests();
void run_minimal_state_applier_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, game_state, state_change, state_change_gate, actor_state, state_value, minimal_applier" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "state smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_state_types_tests();
        return 0;
    } else if (test_name == "game_state") {
        run_game_state_tests();
        return 0;
    } else if (test_name == "state_change") {
        run_state_change_tests();
        return 0;
    } else if (test_name == "state_change_gate") {
        run_state_change_gate_tests();
        return 0;
    } else if (test_name == "actor_state") {
        run_actor_state_tests();
        return 0;
    } else if (test_name == "state_value") {
        run_state_value_tests();
        return 0;
    } else if (test_name == "minimal_applier") {
        run_minimal_state_applier_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
