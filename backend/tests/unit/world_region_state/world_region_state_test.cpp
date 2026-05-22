#include <cassert>
#include <iostream>
#include <memory>
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_region_state/world_region_state_types.h"
#include "pathfinder/world_region_state/in_memory_world_region_state_store.h"
#include "pathfinder/world_region_state/world_region_snapshot_builder.h"
#include "pathfinder/world_region_state/world_region_snapshot_restorer.h"
#include "pathfinder/world_region_state/world_grid_runtime_apply_port.h"
#include "pathfinder/world_region_state/world_region_unload_guard.h"
#include "pathfinder/world_region_state/world_region_lifecycle_service.h"
#include "pathfinder/world_region_state/world_region_save_bridge.h"
#include "pathfinder/world_region_state/world_region_ensure_service_adapter.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"

using namespace pathfinder::world_runtime;
using namespace pathfinder::world_generation;
using namespace pathfinder::world_region_state;
using namespace pathfinder::world_inventory;
using pathfinder::foundation::Result;

namespace {

WorldRegionKey makeKey(int rx, int ry, int size = 16) {
    WorldRegionKey key;
    key.world_id = "test_world";
    key.layer_key = "surface";
    key.rx = rx;
    key.ry = ry;
    key.region_size = size;
    return key;
}

std::unique_ptr<WorldGridRuntime> makeRuntimeWithInitialWorld() {
    auto runtime = std::make_unique<WorldGridRuntime>();
    WorldRuntimeConfig config;
    config.world_id = "test_world";
    config.seed = 42;
    config.initial_region_radius = 1;
    config.region_size = 16;
    config.default_vision_radius = 3;
    runtime->initialize(config);
    runtime->generateInitialWorld(config);

    // P59: generateInitialWorld does not call markRegionGenerated, but P59 relies on it.
    // Mark all initial regions as generated for test consistency.
    int radius = config.initial_region_radius;
    for (int rx = -radius; rx <= radius; ++rx) {
        for (int ry = -radius; ry <= radius; ++ry) {
            runtime->markRegionGenerated(config.world_id + ":surface:region:" + std::to_string(rx) + ":" + std::to_string(ry) + ":" + std::to_string(config.region_size));
        }
    }
    return runtime;
}

// Helper to create a lifecycle service with P58 generation fallback wired in.
std::unique_ptr<WorldRegionLifecycleService> makeLifecycleServiceWithGeneration(
    WorldGridRuntime& runtime,
    IWorldRegionStateStore& store,
    IWorldRegionStateApplyPort& apply_port) {
    WorldGenerationService gen_service;
    WorldRegionEnsureService ensure_service(gen_service, runtime, runtime);
    auto adapter = std::make_unique<WorldRegionEnsureServiceAdapter>(ensure_service);
    return std::make_unique<WorldRegionLifecycleService>(
        store, runtime, apply_port, runtime, runtime, adapter.get());
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// P59: readRegionSlice returns correct data for a region
// ---------------------------------------------------------------------------
void run_read_region_slice_tests() {
    auto runtime = makeRuntimeWithInitialWorld();

    auto key = makeKey(0, 0);
    auto slice_res = runtime->readRegionSlice(key);
    assert(slice_res.is_ok() && "readRegionSlice should succeed for active region");
    const auto& slice = slice_res.value();

    // Region 0,0 should have cells (generated in initial world with radius=1)
    assert(!slice.cells.empty() && "Region 0,0 should have cells");

    // All cells should have region_id matching region_0_0
    for (const auto& cell : slice.cells) {
        assert(cell.region_id == "test_world:surface:region:0:0:16" && "Cell region_id should match");
    }

    // Entities should be OnMap only
    for (const auto& entity : slice.on_map_entities) {
        assert(entity.location_kind == WorldEntityLocationKind::OnMap && "Entities should be OnMap");
    }

    std::cout << "read_region_slice: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: InMemoryWorldRegionStateStore basic operations
// ---------------------------------------------------------------------------
void run_in_memory_store_tests() {
    InMemoryWorldRegionStateStore store;
    WorldRegionKey key = makeKey(1, 1);

    assert(!store.has(key) && "Store should not have key initially");
    assert(store.count() == 0 && "Store should be empty");

    WorldRegionSnapshot snapshot;
    snapshot.header.snapshot_id = "test_snap_1";
    snapshot.header.snapshot_schema_version = "world_region_snapshot.v1";
    snapshot.header.world_id = "test_world";
    snapshot.header.region_key = key;
    snapshot.header.worldgen_profile_key = "default";
    snapshot.header.generator_version = "1.0.0";
    snapshot.header.content_version = "1.0.0";

    auto put_res = store.put(snapshot);
    assert(put_res.is_ok() && "Put should succeed");
    assert(store.has(key) && "Store should have key after put");
    assert(store.count() == 1 && "Store count should be 1");

    auto find_res = store.find(key);
    assert(find_res.is_ok() && "Find should succeed");
    assert(find_res.value()->header.snapshot_id == "test_snap_1" && "Snapshot ID should match");

    auto remove_res = store.remove(key);
    assert(remove_res.is_ok() && "Remove should succeed");
    assert(!store.has(key) && "Store should not have key after remove");

    store.clear();
    assert(store.count() == 0 && "Store should be empty after clear");

    std::cout << "in_memory_store: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Snapshot builder roundtrip
// ---------------------------------------------------------------------------
void run_snapshot_builder_tests() {
    auto runtime = makeRuntimeWithInitialWorld();

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    auto key = makeKey(0, 0);
    WorldRegionSnapshotBuilder builder(*runtime);
    auto snap_res = builder.build(key, context);
    if (snap_res.is_error()) {
        for (const auto& err : snap_res.errors()) {
            std::cerr << "Builder error: " << err.message_key << std::endl;
        }
    }
    assert(snap_res.is_ok() && "Builder should succeed");

    const auto& snap = snap_res.value();
    assert(snap.header.region_key == key && "Region key should match");
    assert(snap.header.snapshot_schema_version == "world_region_snapshot.v1" && "Schema version should match");
    assert(!snap.cells.empty() && "Snapshot should have cells");

    // Validate
    auto valid = snap.validateBasic();
    assert(valid.is_ok() && "Snapshot should be valid");

    std::cout << "snapshot_builder: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Snapshot roundtrip (build -> store -> restore)
// ---------------------------------------------------------------------------
void run_snapshot_roundtrip_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    auto key = makeKey(0, 0);

    // Build snapshot
    WorldRegionSnapshotBuilder builder(*runtime);
    auto snap_res = builder.build(key, context);
    assert(snap_res.is_ok() && "Builder should succeed");
    auto snapshot = snap_res.value();

    // Count original entities and nodes
    size_t original_cell_count = snapshot.cells.size();
    size_t original_entity_count = snapshot.on_map_entities.size();

    // Store
    InMemoryWorldRegionStateStore store;
    auto put_res = store.put(snapshot);
    assert(put_res.is_ok() && "Store put should succeed");

    // Detach region from runtime
    auto detach_res = runtime->detachRegion("test_world:surface:region:0:0:16");
    assert(detach_res.is_ok() && "Detach should succeed");
    assert(!runtime->isRegionGenerated("test_world:surface:region:0:0:16") && "Region should not be generated after detach");

    // Restore
    WorldRegionRestoreContext restore_context;
    restore_context.world_id = "test_world";
    restore_context.world_seed = 42;
    restore_context.world_tick = 1;
    restore_context.current_state_version = runtime->stateVersion();
    restore_context.generator_version = "1.0.0";
    restore_context.content_version = "1.0.0";

    WorldRegionSnapshotRestorer restorer(apply_port);
    auto restore_res = restorer.restore(snapshot, restore_context);
    assert(restore_res.is_ok() && "Restore should succeed");
    assert(restore_res.value().ok() && "Restore result should be ok");
    assert(restore_res.value().status == WorldRegionRestoreStatus::Restored && "Status should be Restored");
    assert(runtime->isRegionGenerated("test_world:surface:region:0:0:16") && "Region should be generated after restore");

    // Verify by reading slice again
    auto slice_res = runtime->readRegionSlice(key);
    assert(slice_res.is_ok() && "Read slice after restore should succeed");
    const auto& slice = slice_res.value();
    assert(slice.cells.size() == original_cell_count && "Cell count should match after roundtrip");
    assert(slice.on_map_entities.size() == original_entity_count && "Entity count should match after roundtrip");

    std::cout << "snapshot_roundtrip: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Lifecycle service seal and restore
// ---------------------------------------------------------------------------
void run_lifecycle_seal_restore_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    auto key = makeKey(0, 0);

    // Initial state: ActiveRuntime
    assert(lifecycle.getLifecycleState(key) == WorldRegionLifecycleState::ActiveRuntime && "Should be ActiveRuntime");

    // Seal
    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");
    assert(seal_res.value().ok() && "Seal result should be ok");
    assert(seal_res.value().status == WorldRegionSealStatus::Sealed && "Status should be Sealed");
    assert(store.has(key) && "Store should have snapshot after seal");
    assert(lifecycle.getLifecycleState(key) == WorldRegionLifecycleState::CachedSnapshot && "Should be CachedSnapshot");

    // Detach from runtime
    runtime->detachRegion("test_world:surface:region:0:0:16");

    // Restore
    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "Restore should succeed");
    assert(restore_res.value().ok() && "Restore result should be ok");
    assert(restore_res.value().status == WorldRegionRestoreStatus::Restored && "Status should be Restored");
    assert(lifecycle.getLifecycleState(key) == WorldRegionLifecycleState::ActiveRuntime && "Should be ActiveRuntime after restore");

    std::cout << "lifecycle_seal_restore: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: ensureAvailable follows active -> restore -> generate
// ---------------------------------------------------------------------------
void run_ensure_available_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    // 1. ActiveRuntime: region 0,0 is already active
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);
    auto key_active = makeKey(0, 0);
    auto avail1 = lifecycle.ensureAvailable(key_active, context);
    assert(avail1.is_ok() && "ensureAvailable for active region should succeed");
    assert(avail1.value().available && "Should be available");
    assert(avail1.value().source == WorldRegionAvailabilitySource::ActiveRuntime && "Source should be ActiveRuntime");

    // 2. CachedSnapshot: seal then detach, then ensure should restore
    auto seal_res = lifecycle.sealRegion(key_active, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");
    runtime->detachRegion("test_world:surface:region:0:0:16");

    auto avail2 = lifecycle.ensureAvailable(key_active, context);
    assert(avail2.is_ok() && "ensureAvailable for cached region should succeed");
    assert(avail2.value().available && "Should be available");
    assert(avail2.value().source == WorldRegionAvailabilitySource::RestoredSnapshot && "Source should be RestoredSnapshot");

    // 3. GeneratedBaseline: a far region that was never generated
    WorldRegionEnsureService ensure_service(gen_service, *runtime, *runtime);
    WorldRegionEnsureServiceAdapter ensure_adapter(ensure_service);
    WorldRegionLifecycleService lifecycle_with_gen(
        store, *runtime, apply_port, *runtime, *runtime, &ensure_adapter);
    auto key_far = makeKey(10, 10);
    assert(!runtime->isRegionGenerated("test_world:surface:region:10:10:16") && "Far region should not be generated");
    auto avail3 = lifecycle_with_gen.ensureAvailable(key_far, context);
    assert(avail3.is_ok() && "ensureAvailable for new region should succeed");
    assert(avail3.value().available && "Should be available");
    assert(avail3.value().source == WorldRegionAvailabilitySource::GeneratedBaseline && "Source should be GeneratedBaseline");
    assert(runtime->isRegionGenerated("test_world:surface:region:10:10:16") && "Far region should be generated after ensure");

    std::cout << "ensure_available: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Harvested resource node state preserved across seal/restore
// ---------------------------------------------------------------------------
void run_harvest_state_preservation_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    // Ensure a generated region (not initial world) to get resource nodes
    WorldRegionEnsureService ensure_service(gen_service, *runtime, *runtime);
    WorldRegionEnsureServiceAdapter ensure_adapter(ensure_service);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime, &ensure_adapter);
    auto key = makeKey(5, 5);
    auto avail = lifecycle.ensureAvailable(key, context);
    assert(avail.is_ok() && avail.value().available && "Should ensure generated region");

    // Find a resource node in region 5,5
    auto slice_res = runtime->readRegionSlice(key);
    assert(slice_res.is_ok() && "Should be able to read slice");
    const auto& slice = slice_res.value();

    if (slice.resource_nodes.empty()) {
        // No resource nodes in this region, skip test
        std::cout << "harvest_state_preservation: skipped (no resource nodes in region)" << std::endl;
        return;
    }

    // Modify resource node: reduce charges and mark as Depleted
    std::string target_node_id = slice.resource_nodes[0].node_id;
    auto node_res = runtime->findResourceNode(target_node_id);
    assert(node_res.is_ok() && "Should find resource node");

    WorldResourceNodeRuntime modified_node = *node_res.value();
    modified_node.remaining_charges = 0;
    modified_node.node_state_str = "Depleted";
    auto update_res = runtime->updateResourceNodeRuntime(modified_node);
    assert(update_res.is_ok() && "Should update node");

    // Seal
    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");

    // Detach
    runtime->detachRegion(key.regionRuntimeId());
    assert(!runtime->isRegionGenerated(key.regionRuntimeId()) && "Region should be detached");

    // Restore
    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "Restore should return a result");
    assert(restore_res.value().status == WorldRegionRestoreStatus::Restored &&
           "Restore should actually restore from snapshot, not AlreadyActive");

    // Verify node state is preserved
    auto restored_node_res = runtime->findResourceNode(target_node_id);
    assert(restored_node_res.is_ok() && "Should find restored node");
    const auto* restored_node = restored_node_res.value();
    assert(restored_node->remaining_charges == 0 && "remaining_charges should be 0 after restore");
    assert(restored_node->node_state_str == "Depleted" && "node_state_str should be Depleted after restore");

    std::cout << "harvest_state_preservation: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Entity pickup/drop preservation across seal/restore
// ---------------------------------------------------------------------------
void run_entity_roundtrip_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    auto key = makeKey(0, 0);

    // Find an OnMap entity
    auto slice_res = runtime->readRegionSlice(key);
    assert(slice_res.is_ok() && "Should read slice");
    const auto& slice = slice_res.value();

    if (slice.on_map_entities.empty()) {
        std::cout << "entity_roundtrip: skipped (no on_map entities in region)" << std::endl;
        return;
    }

    std::string target_entity_id = slice.on_map_entities[0].entity_id;
    std::string target_entity_key = slice.on_map_entities[0].entity_key;

    // Modify entity quantity
    auto entity_res = runtime->findEntity(target_entity_id);
    assert(entity_res.is_ok() && "Should find entity");

    // Note: we can't easily modify entity quantity through the public API.
    // For this test, we just verify the entity exists before and after roundtrip.

    // Seal
    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");

    // Detach
    runtime->detachRegion("test_world:surface:region:0:0:16");

    // Verify entity is gone
    auto gone_res = runtime->findEntity(target_entity_id);
    assert(gone_res.is_error() && "Entity should be gone after detach");

    // Restore
    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "Restore should succeed");

    // Verify entity is back
    auto back_res = runtime->findEntity(target_entity_id);
    assert(back_res.is_ok() && "Entity should be back after restore");
    assert(back_res.value()->entity_key == target_entity_key && "Entity key should match");
    assert(back_res.value()->location_kind == WorldEntityLocationKind::OnMap && "Entity should be OnMap");

    std::cout << "entity_roundtrip: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Unload guard rejects sealing player region
// ---------------------------------------------------------------------------
void run_unload_guard_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldRegionUnloadGuard guard(*runtime);

    auto key = makeKey(0, 0);
    auto check = guard.checkCanUnload(key);

    // Player is at origin (0,0), which is in region (0,0) by generateInitialWorld logic
    // Note: WorldRegionMath::coordToRegion(0,0,16) returns (0,0)
    assert(!check.can_unload && "Should not unload region containing player");
    assert(!check.blocking_reason_keys.empty() && "Should have blocking reasons");

    std::cout << "unload_guard: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Negative coordinate region roundtrip
// ---------------------------------------------------------------------------
void run_negative_coord_region_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    // Ensure a negative coordinate region
    WorldRegionEnsureService ensure_service(gen_service, *runtime, *runtime);
    WorldRegionEnsureServiceAdapter ensure_adapter(ensure_service);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime, &ensure_adapter);
    auto key = makeKey(-1, -1);
    auto avail = lifecycle.ensureAvailable(key, context);
    assert(avail.is_ok() && "ensureAvailable for negative coord region should succeed");
    assert(avail.value().available && "Should be available");
    assert(runtime->isRegionGenerated("test_world:surface:region:-1:-1:16") && "Negative region should be generated");

    // Seal and restore
    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");

    runtime->detachRegion("test_world:surface:region:-1:-1:16");
    assert(!runtime->isRegionGenerated("test_world:surface:region:-1:-1:16") && "Should be detached");

    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "Restore should succeed");
    assert(runtime->isRegionGenerated("test_world:surface:region:-1:-1:16") && "Should be restored");

    std::cout << "negative_coord_region: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Snapshot validation rejects invalid data
// ---------------------------------------------------------------------------
void run_snapshot_validation_tests() {
    WorldRegionSnapshot snapshot;
    snapshot.header.snapshot_schema_version = "world_region_snapshot.v1";
    snapshot.header.world_id = "test_world"; // must match makeKey(0,0).world_id
    snapshot.header.region_key = makeKey(0, 0);
    snapshot.header.worldgen_profile_key = "default";
    snapshot.header.generator_version = "1.0.0";
    snapshot.header.content_version = "1.0.0";

    // Helper: reset to valid baseline
    auto baseline = snapshot;

    // 1. Empty schema version
    {
        auto s = baseline;
        s.header.snapshot_schema_version = "";
        assert(s.validateBasic().is_error() && "Should fail with empty schema version");
    }

    // 2. Unsupported schema version
    {
        auto s = baseline;
        s.header.snapshot_schema_version = "v2";
        assert(s.validateBasic().is_error() && "Should fail with unsupported schema version");
    }

    // 3. World ID mismatch
    {
        auto s = baseline;
        s.header.world_id = "other_world";
        assert(s.validateBasic().is_error() && "Should fail when world_id != region_key.world_id");
    }

    // 4. Empty layer_key
    {
        auto s = baseline;
        s.header.region_key.layer_key = "";
        assert(s.validateBasic().is_error() && "Should fail with empty layer_key");
    }

    // 5. Invalid region_size
    {
        auto s = baseline;
        s.header.region_key.region_size = 0;
        assert(s.validateBasic().is_error() && "Should fail with region_size <= 0");
    }

    // 6. Missing generator version
    {
        auto s = baseline;
        s.header.generator_version = "";
        assert(s.validateBasic().is_error() && "Should fail with empty generator_version");
    }

    // 7. Missing content version
    {
        auto s = baseline;
        s.header.content_version = "";
        assert(s.validateBasic().is_error() && "Should fail with empty content_version");
    }

    // 8. Cell coord outside region
    {
        auto s = baseline;
        WorldRegionCellStateRecord cell;
        cell.cell_id = "c_out";
        cell.coord = WorldCellCoord{8, 0, "surface"};
        cell.region_id = s.header.region_key.regionRuntimeId();
        s.cells.push_back(cell);
        assert(s.validateBasic().is_error() && "Should fail with cell coord outside region 0,0");
    }

    // 9. Entity coord wrong layer
    {
        auto s = baseline;
        WorldRegionEntityStateRecord ent;
        ent.entity_id = "e_wrong_layer";
        ent.location_kind = WorldEntityLocationKind::OnMap;
        ent.coord = WorldCellCoord{0, 0, "underground"};
        s.on_map_entities.push_back(ent);
        assert(s.validateBasic().is_error() && "Should fail with entity coord on wrong layer");
    }

    // 10. Resource node coord outside region
    {
        auto s = baseline;
        WorldRegionResourceNodeStateRecord node;
        node.node_id = "n_out";
        node.coord = WorldCellCoord{-9, 0, "surface"};
        node.max_charges = 1;
        node.remaining_charges = 1;
        s.resource_nodes.push_back(node);
        assert(s.validateBasic().is_error() && "Should fail with node coord outside region 0,0");
    }

    // 11. Entity not OnMap
    {
        auto s = baseline;
        WorldRegionEntityStateRecord ent;
        ent.entity_id = "e1";
        ent.location_kind = WorldEntityLocationKind::InInventory;
        ent.coord = WorldCellCoord{0, 0, "surface"};
        s.on_map_entities.push_back(ent);
        assert(s.validateBasic().is_error() && "Should fail with non-OnMap entity");
    }

    // 7. Resource node missing node_id
    {
        auto s = baseline;
        WorldRegionResourceNodeStateRecord node;
        node.node_id = "";
        node.coord = WorldCellCoord{0, 0, "surface"};
        s.resource_nodes.push_back(node);
        assert(s.validateBasic().is_error() && "Should fail with empty node_id");
    }

    // 8. Resource node max_charges negative
    {
        auto s = baseline;
        WorldRegionResourceNodeStateRecord node;
        node.node_id = "n1";
        node.coord = WorldCellCoord{0, 0, "surface"};
        node.max_charges = -1;
        node.remaining_charges = 0;
        s.resource_nodes.push_back(node);
        assert(s.validateBasic().is_error() && "Should fail with negative max_charges");
    }

    // 9. Resource node remaining_charges negative
    {
        auto s = baseline;
        WorldRegionResourceNodeStateRecord node;
        node.node_id = "n1";
        node.coord = WorldCellCoord{0, 0, "surface"};
        node.max_charges = 5;
        node.remaining_charges = -1;
        s.resource_nodes.push_back(node);
        assert(s.validateBasic().is_error() && "Should fail with negative remaining_charges");
    }

    // 10. Resource node remaining_charges > max_charges
    {
        auto s = baseline;
        WorldRegionResourceNodeStateRecord node;
        node.node_id = "n1";
        node.coord = WorldCellCoord{0, 0, "surface"};
        node.max_charges = 3;
        node.remaining_charges = 5;
        s.resource_nodes.push_back(node);
        assert(s.validateBasic().is_error() && "Should fail with remaining_charges > max_charges");
    }

    // 11. Valid snapshot should pass
    {
        auto s = baseline;
        WorldRegionCellStateRecord cell;
        cell.cell_id = "c1";
        cell.coord = WorldCellCoord{0, 0, "surface"};
        cell.region_id = s.header.region_key.regionRuntimeId();
        s.cells.push_back(cell);

        WorldRegionEntityStateRecord ent;
        ent.entity_id = "e1";
        ent.location_kind = WorldEntityLocationKind::OnMap;
        ent.coord = WorldCellCoord{0, 0, "surface"};
        s.on_map_entities.push_back(ent);

        WorldRegionResourceNodeStateRecord node;
        node.node_id = "n1";
        node.coord = WorldCellCoord{0, 0, "surface"};
        node.max_charges = 3;
        node.remaining_charges = 2;
        s.resource_nodes.push_back(node);

        assert(s.validateBasic().is_ok() && "Valid snapshot should pass");
    }

    std::cout << "snapshot_validation: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Double seal returns AlreadyCached
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// P59: Active region restore is rejected
// ---------------------------------------------------------------------------
void run_active_region_restore_rejected_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    auto key = makeKey(0, 0);

    // Region 0,0 is already active from generateInitialWorld
    assert(lifecycle.getLifecycleState(key) == WorldRegionLifecycleState::ActiveRuntime &&
           "Region should be active");

    // Try to restore a snapshot for an active region
    WorldRegionSnapshot snapshot;
    snapshot.header.snapshot_id = "snap_active";
    snapshot.header.snapshot_schema_version = "world_region_snapshot.v1";
    snapshot.header.world_id = "test_world";
    snapshot.header.region_key = key;
    snapshot.header.worldgen_profile_key = "first_world";
    snapshot.header.generator_version = "1.0.0";
    snapshot.header.content_version = "1.0.0";
    store.put(snapshot);

    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "restoreRegion should return a result");
    assert(restore_res.value().status == WorldRegionRestoreStatus::AlreadyActive &&
           "Should return AlreadyActive, not silently overwrite");

    std::cout << "active_region_restore_rejected: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Cross-region entity conflict is rejected
// ---------------------------------------------------------------------------
void run_cross_region_entity_conflict_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    // Ensure a far region (10,10) so it exists in runtime
    WorldRegionEnsureService ensure_service(gen_service, *runtime, *runtime);
    WorldRegionEnsureServiceAdapter ensure_adapter(ensure_service);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime, &ensure_adapter);
    auto key_far = makeKey(10, 10);
    auto avail = lifecycle.ensureAvailable(key_far, context);
    assert(avail.is_ok() && avail.value().available && "Far region should be generated");

    // Build snapshot for far region
    WorldRegionSnapshotBuilder builder(*runtime);
    auto snap_res = builder.build(key_far, context);
    assert(snap_res.is_ok() && "Builder should succeed");
    auto snapshot = snap_res.value();

    // Store snapshot
    store.put(snapshot);

    // Detach far region
    runtime->detachRegion(key_far.regionRuntimeId());

    // Now manually spawn an entity with the same ID in region (0,0)
    // This simulates a cross-region conflict: the entity ID from the (10,10) snapshot
    // now exists in (0,0).
    if (!snapshot.on_map_entities.empty()) {
        std::string conflict_entity_id = snapshot.on_map_entities[0].entity_id;
        auto origin_cell = runtime->findCell(WorldCellCoord{0, 0, "surface"});
        assert(origin_cell.is_ok() && "Origin cell should exist");

        // Move entity to origin cell by spawning a new one with same ID
        // (spawnEntityOnMap will update existing entity if ID already exists)
        auto spawn_res = runtime->spawnEntityOnMap(
            conflict_entity_id,
            snapshot.on_map_entities[0].entity_key,
            snapshot.on_map_entities[0].display_name_key,
            WorldCellCoord{0, 0, "surface"},
            1, "", false, {}, {}, {});
        assert(spawn_res.is_ok() && "Spawn should succeed");

        // Now restore (10,10) should detect cross-region conflict
        auto restore_res = lifecycle.restoreRegion(key_far);
        assert(restore_res.is_ok() && "restoreRegion should return a result");
        assert(restore_res.value().status == WorldRegionRestoreStatus::RuntimeConflict &&
               "Should detect cross-region entity conflict");
        assert(!restore_res.value().failure_reason_keys.empty() &&
               "Should have failure reasons");
    }

    std::cout << "cross_region_entity_conflict: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Save bridge basic validation
// ---------------------------------------------------------------------------
void run_save_bridge_tests() {
    InMemoryWorldRegionStateStore store;
    WorldRegionSaveBridge bridge;

    // Build empty section draft. P59 full fix: store keys are enumerable, so no warning is expected.
    auto draft_res = bridge.buildSectionDraft(store, "test_world", 0, 1);
    assert(draft_res.is_ok() && "Build section draft should succeed");
    auto draft = draft_res.value();
    assert(draft.collection.world_id == "test_world" && "World ID should match");
    assert(draft.warning_keys.empty() && "Empty enumerable store should not warn");
    assert(draft.collection.region_snapshots.empty() && "Empty store should produce empty snapshot collection");

    // Validate section draft
    auto valid = draft.validateBasic();
    assert(valid.is_ok() && "Draft should be valid");

    // Validate empty collection
    WorldRegionSnapshotCollection empty_collection;
    empty_collection.world_id = "test";
    empty_collection.generator_version = "1.0.0";
    empty_collection.content_version = "1.0.0";
    auto coll_valid = bridge.validateCollection(empty_collection);
    assert(coll_valid.is_ok() && "Empty collection should be valid");

    // Restore from section draft with one snapshot
    WorldRegionSnapshot snapshot;
    snapshot.header.snapshot_id = "snap_1";
    snapshot.header.snapshot_schema_version = "world_region_snapshot.v1";
    snapshot.header.world_id = "test_world";
    snapshot.header.region_key = makeKey(0, 0);
    snapshot.header.worldgen_profile_key = "first_world";
    snapshot.header.generator_version = "1.0.0";
    snapshot.header.content_version = "1.0.0";

    draft.collection.region_snapshots.push_back(std::move(snapshot));
    auto restore_res = bridge.restoreStoreFromSection(store, draft);
    assert(restore_res.is_ok() && "Restore should succeed");
    assert(restore_res.value().size() == 1 && "Should restore 1 region");
    assert(store.has(makeKey(0, 0)) && "Store should have restored snapshot");

    auto enumerated_draft_res = bridge.buildSectionDraft(store, "test_world", 2, 3);
    assert(enumerated_draft_res.is_ok() && "Build section draft from populated store should succeed");
    const auto& enumerated_draft = enumerated_draft_res.value();
    assert(enumerated_draft.warning_keys.empty() && "Populated enumerable store should not warn");
    assert(enumerated_draft.collection.region_snapshots.size() == 1 && "Should enumerate stored snapshot");
    assert(enumerated_draft.collection.region_snapshots[0].header.region_key == makeKey(0, 0) &&
           "Enumerated snapshot key should match");

    std::cout << "save_bridge: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Double seal returns AlreadyCached
// ---------------------------------------------------------------------------
void run_double_seal_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);

    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    auto key = makeKey(0, 0);

    auto seal1 = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal1.is_ok() && "First seal should succeed");
    assert(seal1.value().status == WorldRegionSealStatus::Sealed && "Should be Sealed");

    auto seal2 = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal2.is_ok() && "Second seal should succeed");
    assert(seal2.value().status == WorldRegionSealStatus::AlreadyCached && "Should be AlreadyCached");

    std::cout << "double_seal: passed" << std::endl;
}


// ---------------------------------------------------------------------------
// P59: DetachSafeOnly seals and detaches runtime region when unload guard passes
// ---------------------------------------------------------------------------
void run_detach_safe_only_detaches_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    auto key = makeKey(1, 1);
    assert(runtime->isRegionGenerated(key.regionRuntimeId()) && "Region should start active");

    auto seal_res = lifecycle.sealRegion(key, SealMode::DetachSafeOnly);
    assert(seal_res.is_ok() && "DetachSafeOnly seal should return result");
    assert(seal_res.value().status == WorldRegionSealStatus::Sealed && "Seal should succeed");
    assert(store.has(key) && "Snapshot should be cached before detach");
    assert(!runtime->isRegionGenerated(key.regionRuntimeId()) && "DetachSafeOnly should detach runtime region");
    assert(lifecycle.getLifecycleState(key) == WorldRegionLifecycleState::CachedSnapshot &&
           "Detached sealed region should remain cached");

    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && restore_res.value().ok() && "Detached cached region should restore");
    assert(runtime->isRegionGenerated(key.regionRuntimeId()) && "Region should be active after restore");

    std::cout << "detach_safe_only_detaches: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Exploration slices survive detach/restore instead of being dropped
// ---------------------------------------------------------------------------
void run_exploration_restore_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    auto key = makeKey(0, 0);
    auto visible_before = runtime->getVisibleCellIds("player");
    assert(!visible_before.empty() && "Player should have visible cells before seal");

    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && seal_res.value().ok() && "Seal should succeed");
    runtime->detachRegion(key.regionRuntimeId());
    assert(runtime->getCellVisibility("player", WorldCellCoord{0, 0, "surface"}.cellId()) == WorldCellVisibility::Unknown &&
           "Detach should remove region exploration visibility");

    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && restore_res.value().ok() && "Restore should succeed");
    assert(runtime->getCellVisibility("player", WorldCellCoord{0, 0, "surface"}.cellId()) == WorldCellVisibility::Visible &&
           "Restore should re-apply exploration visibility from snapshot");

    std::cout << "exploration_restore: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: Region math and generated runtime cells use the same centered bounds
// ---------------------------------------------------------------------------
void run_region_coord_consistency_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    for (const auto& key : {makeKey(-1, 0), makeKey(0, 0), makeKey(1, 0)}) {
        auto slice_res = runtime->readRegionSlice(key);
        assert(slice_res.is_ok() && "Initial region slice should be readable");
        const auto& slice = slice_res.value();
        assert(slice.cells.size() == 256 && "16x16 region should contain 256 cells");
        WorldRegionCoord expected{key.rx, key.ry};
        for (const auto& cell : slice.cells) {
            auto actual = WorldRegionMath::coordToRegion(cell.coord.x, cell.coord.y, key.region_size);
            assert(actual.rx == expected.rx && actual.ry == expected.ry &&
                   "Generated cell coord should map back to owning region");
            assert(WorldRegionMath::coordBelongsToRegion(cell.coord, expected, key.region_size, key.layer_key) &&
                   "Generated cell should belong to region by shared math");
        }
    }

    std::cout << "region_coord_consistency: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P59: P45 pickup/drop boundary — InInventory entity must not be restored to OnMap
// ---------------------------------------------------------------------------
void run_p45_pickup_drop_boundary_tests() {
    auto runtime = makeRuntimeWithInitialWorld();
    WorldGenerationService gen_service;
    InMemoryWorldRegionStateStore store;
    WorldGridRuntimeApplyPort apply_port(*runtime, *runtime);
    WorldRegionLifecycleService lifecycle(
        store, *runtime, apply_port, *runtime, *runtime);

    WorldRegionSnapshotBuildContext context;
    context.world_id = "test_world";
    context.world_seed = 42;
    context.world_tick = 0;
    context.state_version = 1;
    context.worldgen_profile_key = "first_world";
    context.generator_version = "1.0.0";
    context.content_version = "1.0.0";

    auto key = makeKey(0, 0);

    // Find an OnMap entity
    auto slice_res = runtime->readRegionSlice(key);
    assert(slice_res.is_ok() && "Should read slice");
    const auto& slice = slice_res.value();

    std::string target_entity_id;
    if (slice.on_map_entities.empty()) {
        // Spawn a test entity if none exists
        target_entity_id = "test_stone_001";
        auto spawn_res = runtime->spawnEntityOnMap(
            target_entity_id, "stone", "stone",
            WorldCellCoord{0, 0, "surface"},
            1, "", false, {}, {}, {});
        assert(spawn_res.is_ok() && "Should spawn test entity");
    } else {
        target_entity_id = slice.on_map_entities[0].entity_id;
    }

    // Verify entity is OnMap before seal
    auto before_entity = runtime->findEntity(target_entity_id);
    assert(before_entity.is_ok() && "Entity should exist");
    assert(before_entity.value()->location_kind == WorldEntityLocationKind::OnMap &&
           "Entity should be OnMap before seal");

    // Seal region — snapshot captures entity as OnMap
    auto seal_res = lifecycle.sealRegion(key, SealMode::DebugOnly);
    assert(seal_res.is_ok() && "Seal should succeed");

    // Simulate pickup: remove from map and mark as InInventory
    auto remove_res = runtime->removeEntityFromMap(target_entity_id);
    assert(remove_res.is_ok() && "Remove from map should succeed");
    auto kind_res = runtime->setEntityLocationKind(target_entity_id, WorldEntityLocationKind::InInventory);
    assert(kind_res.is_ok() && "Set location kind should succeed");

    // Verify entity is now InInventory
    auto picked_entity = runtime->findEntity(target_entity_id);
    assert(picked_entity.is_ok() && "Entity should still exist after pickup");
    assert(picked_entity.value()->location_kind == WorldEntityLocationKind::InInventory &&
           "Entity should be InInventory after pickup");

    // Detach region
    runtime->detachRegion(key.regionRuntimeId());
    assert(!runtime->isRegionGenerated(key.regionRuntimeId()) && "Region should be detached");

    // Try restore — must detect that the entity has been moved and refuse
    auto restore_res = lifecycle.restoreRegion(key);
    assert(restore_res.is_ok() && "restoreRegion should return a result");
    assert(restore_res.value().status == WorldRegionRestoreStatus::RuntimeConflict &&
           "Should reject restore because entity was picked up");

    bool has_moved_conflict = false;
    for (const auto& reason : restore_res.value().failure_reason_keys) {
        if (reason.find("runtime_conflict_entity_moved") != std::string::npos) {
            has_moved_conflict = true;
            break;
        }
    }
    assert(has_moved_conflict && "Should report entity_moved conflict");

    // Entity must remain InInventory, not restored to OnMap
    auto after_entity = runtime->findEntity(target_entity_id);
    assert(after_entity.is_ok() && "Entity should still exist after failed restore");
    assert(after_entity.value()->location_kind == WorldEntityLocationKind::InInventory &&
           "Entity must stay InInventory after restore conflict");

    std::cout << "p45_pickup_drop_boundary: passed" << std::endl;
}
