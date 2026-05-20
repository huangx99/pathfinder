#include <iostream>
#include <string>

// Forward declarations
void run_world_generation_same_seed_same_region_same_result_tests();
void run_world_generation_different_seed_different_distribution_tests();
void run_world_generation_manifest_contains_seed_profile_version_tests();
void run_world_generation_creates_cells_with_layer_key_tests();
void run_world_generation_cells_have_valid_terrain_tests();
void run_world_generation_spawn_area_not_blocked_tests();
void run_world_generation_places_resource_nodes_on_allowed_terrain_tests();
void run_world_generation_resource_node_has_action_and_output_keys_tests();
void run_world_generation_resource_node_id_is_stable_tests();
void run_world_generation_spawn_safety_has_basic_food_tests();
void run_world_generation_spawn_safety_has_basic_materials_tests();
void run_world_generation_spawn_safety_blocks_immediate_active_threat_tests();
void run_world_generation_ground_items_are_on_map_only_tests();
void run_world_generation_ground_items_have_quantity_stack_key_tests();
void run_world_generation_does_not_create_player_inventory_items_tests();
void run_world_command_generate_world_uses_world_generation_service_tests();
void run_world_generation_same_seed_generates_same_manifest_tests();
void run_world_generation_adjacent_regions_no_id_collision_tests();
void run_world_generation_cells_created_in_runtime_tests();
void run_world_generation_entity_attached_to_cell_tests();
void run_world_generation_resource_nodes_in_runtime_tests();
void run_world_generation_manifest_timestamp_deterministic_tests();
void run_world_generation_generate_is_pure_tests();
void run_world_generation_repeated_apply_blocked_tests();
void run_world_generation_spawn_safety_uses_tags_not_fixed_keys_tests();
void run_world_generation_command_handler_parsing_tests();
void run_world_generation_spawn_entity_missing_cell_fails_tests();
void run_world_generation_ground_items_non_empty_for_first_world_tests();
void run_world_generation_resource_node_missing_cell_fails_tests();
void run_world_generation_resource_nodes_applied_to_existing_cells_tests();
void run_world_generation_invalid_layer_rejected_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, same_seed_same_result, different_seed_distribution, "
                   "manifest_seed_profile, cells_with_layer_key, valid_terrain, spawn_not_blocked, "
                   "resource_on_allowed_terrain, resource_action_output, resource_node_id_stable, "
                   "spawn_safety_food, spawn_safety_materials, spawn_safety_no_threat, "
                   "ground_items_on_map, ground_items_stack, no_inventory_items, "
                   "command_uses_service, same_seed_manifest, "
                   "adjacent_regions_no_collision, cells_created_in_runtime, entity_attached_to_cell, "
                   "resource_nodes_in_runtime, manifest_timestamp_deterministic, generate_is_pure, "
                   "repeated_apply_blocked, spawn_safety_uses_tags, command_handler_parsing, "
                   "spawn_entity_missing_cell_fails, ground_items_non_empty, "
                   "resource_node_missing_cell_fails, resource_nodes_applied_to_existing_cells, "
                   "invalid_layer_rejected" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_generation smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "same_seed_same_result") {
        run_world_generation_same_seed_same_region_same_result_tests();
    } else if (test_name == "different_seed_distribution") {
        run_world_generation_different_seed_different_distribution_tests();
    } else if (test_name == "manifest_seed_profile") {
        run_world_generation_manifest_contains_seed_profile_version_tests();
    } else if (test_name == "cells_with_layer_key") {
        run_world_generation_creates_cells_with_layer_key_tests();
    } else if (test_name == "valid_terrain") {
        run_world_generation_cells_have_valid_terrain_tests();
    } else if (test_name == "spawn_not_blocked") {
        run_world_generation_spawn_area_not_blocked_tests();
    } else if (test_name == "resource_on_allowed_terrain") {
        run_world_generation_places_resource_nodes_on_allowed_terrain_tests();
    } else if (test_name == "resource_action_output") {
        run_world_generation_resource_node_has_action_and_output_keys_tests();
    } else if (test_name == "resource_node_id_stable") {
        run_world_generation_resource_node_id_is_stable_tests();
    } else if (test_name == "spawn_safety_food") {
        run_world_generation_spawn_safety_has_basic_food_tests();
    } else if (test_name == "spawn_safety_materials") {
        run_world_generation_spawn_safety_has_basic_materials_tests();
    } else if (test_name == "spawn_safety_no_threat") {
        run_world_generation_spawn_safety_blocks_immediate_active_threat_tests();
    } else if (test_name == "ground_items_on_map") {
        run_world_generation_ground_items_are_on_map_only_tests();
    } else if (test_name == "ground_items_stack") {
        run_world_generation_ground_items_have_quantity_stack_key_tests();
    } else if (test_name == "no_inventory_items") {
        run_world_generation_does_not_create_player_inventory_items_tests();
    } else if (test_name == "command_uses_service") {
        run_world_command_generate_world_uses_world_generation_service_tests();
    } else if (test_name == "same_seed_manifest") {
        run_world_generation_same_seed_generates_same_manifest_tests();
    } else if (test_name == "adjacent_regions_no_collision") {
        run_world_generation_adjacent_regions_no_id_collision_tests();
    } else if (test_name == "cells_created_in_runtime") {
        run_world_generation_cells_created_in_runtime_tests();
    } else if (test_name == "entity_attached_to_cell") {
        run_world_generation_entity_attached_to_cell_tests();
    } else if (test_name == "resource_nodes_in_runtime") {
        run_world_generation_resource_nodes_in_runtime_tests();
    } else if (test_name == "manifest_timestamp_deterministic") {
        run_world_generation_manifest_timestamp_deterministic_tests();
    } else if (test_name == "generate_is_pure") {
        run_world_generation_generate_is_pure_tests();
    } else if (test_name == "repeated_apply_blocked") {
        run_world_generation_repeated_apply_blocked_tests();
    } else if (test_name == "spawn_safety_uses_tags") {
        run_world_generation_spawn_safety_uses_tags_not_fixed_keys_tests();
    } else if (test_name == "command_handler_parsing") {
        run_world_generation_command_handler_parsing_tests();
    } else if (test_name == "spawn_entity_missing_cell_fails") {
        run_world_generation_spawn_entity_missing_cell_fails_tests();
    } else if (test_name == "ground_items_non_empty") {
        run_world_generation_ground_items_non_empty_for_first_world_tests();
    } else if (test_name == "resource_node_missing_cell_fails") {
        run_world_generation_resource_node_missing_cell_fails_tests();
    } else if (test_name == "resource_nodes_applied_to_existing_cells") {
        run_world_generation_resource_nodes_applied_to_existing_cells_tests();
    } else if (test_name == "invalid_layer_rejected") {
        run_world_generation_invalid_layer_rejected_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
