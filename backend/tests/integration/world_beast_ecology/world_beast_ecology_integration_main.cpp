#include <cassert>
#include <iostream>
#include <string>

void run_world_beast_ecology_integration_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available: integration" << std::endl;
        return 1;
    }
    std::string test_name = argv[1];
    if (test_name == "integration") {
        run_world_beast_ecology_integration_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
    return 0;
}
