#include <iostream>
#include <string>

void run_activity_window_tests();
void run_lifecycle_trigger_tests();
void run_map_selection_tests();
void run_map_projection_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, activity_window, lifecycle_trigger, "
                   "map_selection, map_projection" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_map_interaction smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "activity_window") {
        run_activity_window_tests();
    } else if (test_name == "lifecycle_trigger") {
        run_lifecycle_trigger_tests();
    } else if (test_name == "map_selection") {
        run_map_selection_tests();
    } else if (test_name == "map_projection") {
        run_map_projection_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
