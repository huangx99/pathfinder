#include <iostream>
#include <string>

// Forward declarations of test functions
void run_cognition_types_tests();
void run_cognition_state_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, cognition_state" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "cognition smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_cognition_types_tests();
        return 0;
    } else if (test_name == "cognition_state") {
        run_cognition_state_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
