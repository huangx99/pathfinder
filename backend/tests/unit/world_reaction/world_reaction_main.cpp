#include <iostream>
#include <string>

// Test function declarations
void run_world_reaction_apply_craft_torch_to_inventory_tests();
void run_world_reaction_apply_craft_axe_to_inventory_tests();
void run_world_reaction_validate_input_missing_tests();
void run_world_reaction_validate_state_mismatch_tests();
void run_world_reaction_validate_quantity_insufficient_tests();
void run_world_reaction_validate_catalyst_not_consumed_tests();
void run_world_reaction_failure_does_not_pollute_state_tests();
void run_world_reaction_craft_output_entity_exists_in_world_runtime_tests();
void run_world_reaction_output_id_is_command_deterministic_tests();
void run_world_reaction_consumes_same_cell_map_input_tests();
void run_world_reaction_consumes_adjacent_map_input_tests();
void run_world_reaction_drop_on_map_output_spawns_entity_on_actor_cell_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests:" << std::endl;
        std::cout << "  apply_craft_torch_to_inventory" << std::endl;
        std::cout << "  apply_craft_axe_to_inventory" << std::endl;
        std::cout << "  validate_input_missing" << std::endl;
        std::cout << "  validate_state_mismatch" << std::endl;
        std::cout << "  validate_quantity_insufficient" << std::endl;
        std::cout << "  validate_catalyst_not_consumed" << std::endl;
        std::cout << "  failure_does_not_pollute_state" << std::endl;
        std::cout << "  craft_output_entity_exists_in_world_runtime" << std::endl;
        std::cout << "  output_id_is_command_deterministic" << std::endl;
        std::cout << "  consumes_same_cell_map_input" << std::endl;
        std::cout << "  consumes_adjacent_map_input" << std::endl;
        std::cout << "  drop_on_map_output_spawns_entity_on_actor_cell" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "apply_craft_torch_to_inventory") {
        run_world_reaction_apply_craft_torch_to_inventory_tests();
    } else if (test_name == "apply_craft_axe_to_inventory") {
        run_world_reaction_apply_craft_axe_to_inventory_tests();
    } else if (test_name == "validate_input_missing") {
        run_world_reaction_validate_input_missing_tests();
    } else if (test_name == "validate_state_mismatch") {
        run_world_reaction_validate_state_mismatch_tests();
    } else if (test_name == "validate_quantity_insufficient") {
        run_world_reaction_validate_quantity_insufficient_tests();
    } else if (test_name == "validate_catalyst_not_consumed") {
        run_world_reaction_validate_catalyst_not_consumed_tests();
    } else if (test_name == "failure_does_not_pollute_state") {
        run_world_reaction_failure_does_not_pollute_state_tests();
    } else if (test_name == "craft_output_entity_exists_in_world_runtime") {
        run_world_reaction_craft_output_entity_exists_in_world_runtime_tests();
    } else if (test_name == "output_id_is_command_deterministic") {
        run_world_reaction_output_id_is_command_deterministic_tests();
    } else if (test_name == "consumes_same_cell_map_input") {
        run_world_reaction_consumes_same_cell_map_input_tests();
    } else if (test_name == "consumes_adjacent_map_input") {
        run_world_reaction_consumes_adjacent_map_input_tests();
    } else if (test_name == "drop_on_map_output_spawns_entity_on_actor_cell") {
        run_world_reaction_drop_on_map_output_spawns_entity_on_actor_cell_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
