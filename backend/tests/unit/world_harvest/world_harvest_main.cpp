#include <iostream>
#include <string>

// Forward declarations
void run_world_harvest_enum_to_string_tests();
void run_world_harvest_request_validate_basic_tests();
void run_world_harvest_kind_from_command_kind_tests();
void run_world_harvest_validate_actor_missing_tests();
void run_world_harvest_validate_node_missing_tests();
void run_world_harvest_validate_node_depleted_tests();
void run_world_harvest_validate_action_mismatch_tests();
void run_world_harvest_validate_too_far_tests();
void run_world_harvest_validate_not_visible_tests();
void run_world_harvest_validate_tool_missing_tests();
void run_world_harvest_validate_tool_present_tests();
void run_world_harvest_tool_durability_decrements_tests();
void run_world_harvest_worn_out_tool_rejected_tests();
void run_world_harvest_apply_gather_creates_output_on_map_tests();
void run_world_harvest_apply_chop_creates_wood_on_map_tests();
void run_world_harvest_apply_consumes_one_charge_tests();
void run_world_harvest_apply_depletes_node_at_zero_tests();
void run_world_harvest_apply_output_id_deterministic_tests();
void run_world_harvest_apply_failure_does_not_change_node_tests();
void run_world_harvest_apply_failure_does_not_spawn_output_tests();
void run_world_harvest_output_uses_p45_on_map_location_tests();
void run_world_harvest_output_not_in_actor_inventory_tests();
void run_world_harvest_output_cell_contains_entity_id_tests();
void run_world_harvest_output_can_be_picked_up_by_p45_pickup_tests();
void run_world_harvest_rejects_prefer_inventory_policy_tests();
void run_world_harvest_rejects_empty_outputs_tests();
void run_world_harvest_rejects_empty_output_key_tests();
void run_world_harvest_rejects_unknown_policy_string_tests();
void run_world_harvest_command_rejects_bad_policy_parameter_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, enum_to_string, request_validate_basic, kind_from_command_kind, "
                   "validate_actor_missing, validate_node_missing, validate_node_depleted, "
                   "validate_action_mismatch, validate_too_far, validate_not_visible, "
                   "validate_tool_missing, validate_tool_present, tool_durability_decrements, worn_out_tool_rejected, "
                   "apply_gather_creates_output_on_map, apply_chop_creates_wood_on_map, "
                   "apply_consumes_one_charge, apply_depletes_node_at_zero, "
                   "apply_output_id_deterministic, apply_failure_does_not_change_node, "
                   "apply_failure_does_not_spawn_output, "
                   "output_uses_p45_on_map_location, output_not_in_actor_inventory, "
                   "output_cell_contains_entity_id, output_can_be_picked_up_by_p45_pickup, "
                   "rejects_prefer_inventory_policy, rejects_empty_outputs, rejects_empty_output_key, "
                   "rejects_unknown_policy_string, command_rejects_bad_policy_parameter" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_harvest smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "enum_to_string") {
        run_world_harvest_enum_to_string_tests();
    } else if (test_name == "request_validate_basic") {
        run_world_harvest_request_validate_basic_tests();
    } else if (test_name == "kind_from_command_kind") {
        run_world_harvest_kind_from_command_kind_tests();
    } else if (test_name == "validate_actor_missing") {
        run_world_harvest_validate_actor_missing_tests();
    } else if (test_name == "validate_node_missing") {
        run_world_harvest_validate_node_missing_tests();
    } else if (test_name == "validate_node_depleted") {
        run_world_harvest_validate_node_depleted_tests();
    } else if (test_name == "validate_action_mismatch") {
        run_world_harvest_validate_action_mismatch_tests();
    } else if (test_name == "validate_too_far") {
        run_world_harvest_validate_too_far_tests();
    } else if (test_name == "validate_not_visible") {
        run_world_harvest_validate_not_visible_tests();
    } else if (test_name == "validate_tool_missing") {
        run_world_harvest_validate_tool_missing_tests();
    } else if (test_name == "validate_tool_present") {
        run_world_harvest_validate_tool_present_tests();
    } else if (test_name == "tool_durability_decrements") {
        run_world_harvest_tool_durability_decrements_tests();
    } else if (test_name == "worn_out_tool_rejected") {
        run_world_harvest_worn_out_tool_rejected_tests();
    } else if (test_name == "apply_gather_creates_output_on_map") {
        run_world_harvest_apply_gather_creates_output_on_map_tests();
    } else if (test_name == "apply_chop_creates_wood_on_map") {
        run_world_harvest_apply_chop_creates_wood_on_map_tests();
    } else if (test_name == "apply_consumes_one_charge") {
        run_world_harvest_apply_consumes_one_charge_tests();
    } else if (test_name == "apply_depletes_node_at_zero") {
        run_world_harvest_apply_depletes_node_at_zero_tests();
    } else if (test_name == "apply_output_id_deterministic") {
        run_world_harvest_apply_output_id_deterministic_tests();
    } else if (test_name == "apply_failure_does_not_change_node") {
        run_world_harvest_apply_failure_does_not_change_node_tests();
    } else if (test_name == "apply_failure_does_not_spawn_output") {
        run_world_harvest_apply_failure_does_not_spawn_output_tests();
    } else if (test_name == "output_uses_p45_on_map_location") {
        run_world_harvest_output_uses_p45_on_map_location_tests();
    } else if (test_name == "output_not_in_actor_inventory") {
        run_world_harvest_output_not_in_actor_inventory_tests();
    } else if (test_name == "output_cell_contains_entity_id") {
        run_world_harvest_output_cell_contains_entity_id_tests();
    } else if (test_name == "output_can_be_picked_up_by_p45_pickup") {
        run_world_harvest_output_can_be_picked_up_by_p45_pickup_tests();
    } else if (test_name == "rejects_prefer_inventory_policy") {
        run_world_harvest_rejects_prefer_inventory_policy_tests();
    } else if (test_name == "rejects_empty_outputs") {
        run_world_harvest_rejects_empty_outputs_tests();
    } else if (test_name == "rejects_empty_output_key") {
        run_world_harvest_rejects_empty_output_key_tests();
    } else if (test_name == "rejects_unknown_policy_string") {
        run_world_harvest_rejects_unknown_policy_string_tests();
    } else if (test_name == "command_rejects_bad_policy_parameter") {
        run_world_harvest_command_rejects_bad_policy_parameter_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
