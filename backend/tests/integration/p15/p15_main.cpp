#include <iostream>
#include <string>

// Forward declarations
void run_p15_cognition_eat_feedback_flow_tests();
void run_p15_cognition_use_feedback_flow_tests();
void run_p15_cognition_hidden_truth_boundary_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available: smoke, eat_feedback, use_feedback, hidden_truth" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "p15 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "eat_feedback") {
        run_p15_cognition_eat_feedback_flow_tests();
        return 0;
    } else if (test_name == "use_feedback") {
        run_p15_cognition_use_feedback_flow_tests();
        return 0;
    } else if (test_name == "hidden_truth") {
        run_p15_cognition_hidden_truth_boundary_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
