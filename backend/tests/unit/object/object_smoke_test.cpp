#include <iostream>
#include <string>

// Forward declarations of test functions
void run_object_types_tests();
void run_world_object_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, world_object" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "object smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_object_types_tests();
        return 0;
    } else if (test_name == "world_object") {
        run_world_object_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
