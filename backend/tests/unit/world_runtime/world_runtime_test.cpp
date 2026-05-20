#include <iostream>
#include <string>

void run_coordinate_id_tests();
void run_coordinate_comparison_tests();
void run_enum_tests();
void run_generate_deterministic_tests();
void run_find_actor_tests();
void run_move_adjacent_tests();
void run_move_blocked_tests();
void run_move_not_adjacent_tests();
void run_move_out_of_bounds_tests();
void run_visibility_update_tests();
void run_time_step_tests();
void run_actor_missing_tests();
void run_inspect_tests();
void run_entity_location_tests();
void run_deterministic_entity_ids_tests();
void run_origin_contains_player_entity_tests();
void run_projection_cell_tests();
void run_projection_entity_tests();
void run_projection_no_unknown_cells_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, coordinate_id, coordinate_comparison, enum, "
                   "generate_deterministic, find_actor, move_adjacent, move_blocked, "
                   "move_not_adjacent, move_out_of_bounds, visibility_update, time_step, "
                   "actor_missing, inspect, entity_location, deterministic_entity_ids, "
                   "origin_contains_player_entity, projection_cell, projection_entity, "
                   "projection_no_unknown_cells" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_runtime smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "coordinate_id") {
        run_coordinate_id_tests();
    } else if (test_name == "coordinate_comparison") {
        run_coordinate_comparison_tests();
    } else if (test_name == "enum") {
        run_enum_tests();
    } else if (test_name == "generate_deterministic") {
        run_generate_deterministic_tests();
    } else if (test_name == "find_actor") {
        run_find_actor_tests();
    } else if (test_name == "move_adjacent") {
        run_move_adjacent_tests();
    } else if (test_name == "move_blocked") {
        run_move_blocked_tests();
    } else if (test_name == "move_not_adjacent") {
        run_move_not_adjacent_tests();
    } else if (test_name == "move_out_of_bounds") {
        run_move_out_of_bounds_tests();
    } else if (test_name == "visibility_update") {
        run_visibility_update_tests();
    } else if (test_name == "time_step") {
        run_time_step_tests();
    } else if (test_name == "actor_missing") {
        run_actor_missing_tests();
    } else if (test_name == "inspect") {
        run_inspect_tests();
    } else if (test_name == "entity_location") {
        run_entity_location_tests();
    } else if (test_name == "deterministic_entity_ids") {
        run_deterministic_entity_ids_tests();
    } else if (test_name == "origin_contains_player_entity") {
        run_origin_contains_player_entity_tests();
    } else if (test_name == "projection_cell") {
        run_projection_cell_tests();
    } else if (test_name == "projection_entity") {
        run_projection_entity_tests();
    } else if (test_name == "projection_no_unknown_cells") {
        run_projection_no_unknown_cells_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
