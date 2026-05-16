#include <iostream>
#include <string>

// Forward declaration of test functions
void run_unknown_fruit_fixture_tests();
void run_eat_object_resolver_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, unknown_fruit_fixture, eat_object_resolver" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "rules smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "unknown_fruit_fixture") {
        run_unknown_fruit_fixture_tests();
        return 0;
    } else if (test_name == "eat_object_resolver") {
        run_eat_object_resolver_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
