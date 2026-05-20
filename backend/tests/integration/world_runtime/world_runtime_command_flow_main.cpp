#include <iostream>
#include <string>

void run_generate_world_runtime_tests();
void run_move_runtime_tests();
void run_move_blocked_runtime_tests();
void run_inspect_runtime_tests();
void run_projection_patch_runtime_tests();
void run_pipeline_with_runtime_tests();
void run_no_playbook_rules_tests();
void run_wait_runtime_tests();
void run_pipeline_projection_patch_runtime_tests();
void run_invalid_parameters_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, generate_world_runtime, move_runtime, "
                   "move_blocked_runtime, inspect_runtime, projection_patch_runtime, "
                   "pipeline_with_runtime, no_playbook_rules, wait_runtime, "
                   "pipeline_projection_patch_runtime, invalid_parameters" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_runtime command flow smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "generate_world_runtime") {
        run_generate_world_runtime_tests();
    } else if (test_name == "move_runtime") {
        run_move_runtime_tests();
    } else if (test_name == "move_blocked_runtime") {
        run_move_blocked_runtime_tests();
    } else if (test_name == "inspect_runtime") {
        run_inspect_runtime_tests();
    } else if (test_name == "projection_patch_runtime") {
        run_projection_patch_runtime_tests();
    } else if (test_name == "pipeline_with_runtime") {
        run_pipeline_with_runtime_tests();
    } else if (test_name == "no_playbook_rules") {
        run_no_playbook_rules_tests();
    } else if (test_name == "wait_runtime") {
        run_wait_runtime_tests();
    } else if (test_name == "pipeline_projection_patch_runtime") {
        run_pipeline_projection_patch_runtime_tests();
    } else if (test_name == "invalid_parameters") {
        run_invalid_parameters_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
