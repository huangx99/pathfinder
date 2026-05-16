#include <iostream>
#include <string>

// Forward declarations of test functions
void run_pipeline_types_tests();
void run_pipeline_context_tests();
void run_pipeline_result_tests();
void run_rule_pipeline_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, types, context, result, rule_pipeline" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "pipeline smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "types") {
        run_pipeline_types_tests();
        return 0;
    } else if (test_name == "context") {
        run_pipeline_context_tests();
        return 0;
    } else if (test_name == "result") {
        run_pipeline_result_tests();
        return 0;
    } else if (test_name == "rule_pipeline") {
        run_rule_pipeline_tests();
        return 0;
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }
}
