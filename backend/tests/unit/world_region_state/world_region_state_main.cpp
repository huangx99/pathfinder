#include <iostream>
#include <string>

void run_read_region_slice_tests();
void run_in_memory_store_tests();
void run_snapshot_builder_tests();
void run_snapshot_roundtrip_tests();
void run_active_region_restore_rejected_tests();
void run_cross_region_entity_conflict_tests();
void run_lifecycle_seal_restore_tests();
void run_ensure_available_tests();
void run_harvest_state_preservation_tests();
void run_entity_roundtrip_tests();
void run_unload_guard_tests();
void run_negative_coord_region_tests();
void run_snapshot_validation_tests();
void run_save_bridge_tests();
void run_double_seal_tests();
void run_p45_pickup_drop_boundary_tests();
void run_detach_safe_only_detaches_tests();
void run_exploration_restore_tests();
void run_region_coord_consistency_tests();

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, read_region_slice, in_memory_store, "
                   "snapshot_builder, snapshot_roundtrip, lifecycle_seal_restore, "
                   "ensure_available, harvest_state_preservation, entity_roundtrip, "
                   "unload_guard, negative_coord_region, snapshot_validation, save_bridge, double_seal, detach_safe_only_detaches, exploration_restore, region_coord_consistency, p45_pickup_drop_boundary"
                   << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_region_state smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "read_region_slice") {
        run_read_region_slice_tests();
    } else if (test_name == "in_memory_store") {
        run_in_memory_store_tests();
    } else if (test_name == "snapshot_builder") {
        run_snapshot_builder_tests();
    } else if (test_name == "snapshot_roundtrip") {
        run_snapshot_roundtrip_tests();
    } else if (test_name == "active_region_restore_rejected") {
        run_active_region_restore_rejected_tests();
    } else if (test_name == "cross_region_entity_conflict") {
        run_cross_region_entity_conflict_tests();
    } else if (test_name == "lifecycle_seal_restore") {
        run_lifecycle_seal_restore_tests();
    } else if (test_name == "ensure_available") {
        run_ensure_available_tests();
    } else if (test_name == "harvest_state_preservation") {
        run_harvest_state_preservation_tests();
    } else if (test_name == "entity_roundtrip") {
        run_entity_roundtrip_tests();
    } else if (test_name == "unload_guard") {
        run_unload_guard_tests();
    } else if (test_name == "negative_coord_region") {
        run_negative_coord_region_tests();
    } else if (test_name == "snapshot_validation") {
        run_snapshot_validation_tests();
    } else if (test_name == "save_bridge") {
        run_save_bridge_tests();
    } else if (test_name == "double_seal") {
        run_double_seal_tests();
    } else if (test_name == "detach_safe_only_detaches") {
        run_detach_safe_only_detaches_tests();
    } else if (test_name == "exploration_restore") {
        run_exploration_restore_tests();
    } else if (test_name == "region_coord_consistency") {
        run_region_coord_consistency_tests();
    } else if (test_name == "p45_pickup_drop_boundary") {
        run_p45_pickup_drop_boundary_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
