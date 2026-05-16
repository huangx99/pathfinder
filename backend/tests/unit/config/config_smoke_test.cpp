#include <cassert>
#include <iostream>
#include <string>

// Forward declarations of test functions
void run_config_validation_tests();
void run_config_package_tests();
void run_config_loader_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, validation, package, loader" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "config smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "validation") {
        run_config_validation_tests();
        return 0;
    } else if (test_name == "package") {
        run_config_package_tests();
        return 0;
    } else if (test_name == "loader") {
        run_config_loader_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
