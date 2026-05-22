#include <iostream>
#include <string>

void run_map_interaction_seal_restore_flow_tests();
void run_map_interaction_inventory_option_flow_tests();
void run_map_interaction_stale_selection_no_snapshot_pollution_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, map_interaction_seal_restore_flow, map_interaction_inventory_option_flow, map_interaction_stale_selection_no_snapshot_pollution" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "p60 integration smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "map_interaction_seal_restore_flow") {
        run_map_interaction_seal_restore_flow_tests();
    } else if (test_name == "map_interaction_inventory_option_flow") {
        run_map_interaction_inventory_option_flow_tests();
    } else if (test_name == "map_interaction_stale_selection_no_snapshot_pollution") {
        run_map_interaction_stale_selection_no_snapshot_pollution_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
