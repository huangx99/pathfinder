#include <iostream>
#include <string>

// Forward declaration of test functions
void run_p3_integration_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, integration" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "p3 smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "integration") {
        run_p3_integration_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
