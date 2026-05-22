#include <cassert>
#include <iostream>
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"

using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::client_runtime_bridge;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
struct RegionEnsureFixture {
    WorldGridRuntime runtime;
    WorldGenerationService gen_service;
    WorldRegionEnsureService ensure_service;
    ClientWorldRegionEnsureAdapter ensure_adapter;

    RegionEnsureFixture()
        : ensure_service(gen_service, runtime, runtime)
        , ensure_adapter(ensure_service, runtime) {
        WorldRuntimeConfig config;
        config.world_id = "test_world";
        config.seed = 42;
        config.region_size = 16;
        config.default_vision_radius = 3;
        runtime.initialize(config);

        // Pre-generate origin region only
        WorldGenerationRequest req;
        req.world_id = config.world_id;
        req.world_seed = config.seed;
        req.generator_version = "1.0.0";
        req.content_version = "1.0.0";
        req.worldgen_profile_key = "first_world";
        req.region_coord = WorldRegionCoord{0, 0};
        req.region_size = config.region_size;
        req.enabled_layer_keys = {"surface"};
        auto result = gen_service.generate(req);
        if (result.ok) {
            WorldGenerationApplier applier(runtime, runtime);
            applier.apply(result);
        }
        runtime.setupPlayerActor(config);
    }
};

// ---------------------------------------------------------------------------
// Idempotency: ensure same region twice does not duplicate cells
// ---------------------------------------------------------------------------
void run_ensure_idempotency_tests() {
    RegionEnsureFixture f;

    WorldRegionEnsureRequest req;
    req.world_id = "test_world";
    req.world_seed = 42;
    req.generator_version = "1.0.0";
    req.content_version = "1.0.0";
    req.worldgen_profile_key = "first_world";
    req.layer_key = "surface";
    req.region_size = 16;
    req.ensure_kind = WorldRegionEnsureKind::TestOnly;
    req.coverage_mode = WorldRegionCoverageMode::RegionList;
    req.explicit_regions = {WorldRegionCoord{1, 0}};
    req.max_regions_to_generate = 9;
    req.allow_generate = true;

    auto snap_before = f.runtime.snapshotForDebug();
    int cells_before = static_cast<int>(snap_before.value().cells.size());
    int entities_before = static_cast<int>(snap_before.value().entities.size());
    int nodes_before = static_cast<int>(snap_before.value().resource_nodes.size());

    auto res1 = f.ensure_service.ensureRegions(req);
    assert(res1.is_ok());
    assert(res1.value().ok);
    assert(res1.value().generated_region_count == 1);

    auto snap_mid = f.runtime.snapshotForDebug();
    int cells_mid = static_cast<int>(snap_mid.value().cells.size());
    assert(cells_mid > cells_before);

    // Second ensure: should be AlreadyAvailable
    auto res2 = f.ensure_service.ensureRegions(req);
    assert(res2.is_ok());
    assert(res2.value().ok);
    assert(res2.value().already_available_count == 1);
    assert(res2.value().generated_region_count == 0);

    auto snap_after = f.runtime.snapshotForDebug();
    int cells_after = static_cast<int>(snap_after.value().cells.size());
    int entities_after = static_cast<int>(snap_after.value().entities.size());
    int nodes_after = static_cast<int>(snap_after.value().resource_nodes.size());

    assert(cells_after == cells_mid);
    assert(entities_after == static_cast<int>(snap_mid.value().entities.size()));
    assert(nodes_after == static_cast<int>(snap_mid.value().resource_nodes.size()));

    std::cout << "world_region_ensure_idempotency: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Boundary move: ensure target region before move option
// Player at x=7 (region 0 right edge). ensureForAvailableCommands must generate region (1,0).
// ---------------------------------------------------------------------------
void run_boundary_move_tests() {
    RegionEnsureFixture f;

    // Walk player from (0,0) to (7,0) one step at a time (moveActor requires adjacency)
    for (int x = 0; x < 7; ++x) {
        auto move_res = f.runtime.moveActor("player", WorldCellCoord{x + 1, 0, "surface"});
        assert(move_res.is_ok());
        assert(move_res.value().moved);
    }
    // Now player is at x=7,y=0 (region (0,0) right edge, x=8 is in region (1,0))

    // Now ensure neighbor moves: target (8,0) is in region (1,0) which is not yet generated
    auto res = f.ensure_adapter.ensureForAvailableCommands("player", "surface");
    assert(res.is_ok());
    assert(res.value().ok);

    // Must have generated region (1,0)
    bool has_r10 = false;
    for (const auto& item : res.value().item_results) {
        if (item.region_key.rx == 1 && item.region_key.ry == 0) {
            has_r10 = true;
            assert(item.status == WorldRegionEnsureStatus::GeneratedAndApplied ||
                   item.status == WorldRegionEnsureStatus::AlreadyAvailable);
        }
    }
    assert(has_r10); // strict: region (1,0) must be ensured

    // Cell (8,0) must now exist
    auto cell_res = f.runtime.findCell(WorldCellCoord{8, 0, "surface"});
    assert(cell_res.is_ok());

    std::cout << "world_region_ensure_boundary_move: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Negative coord move: ensure (-1,0) region works with floorDiv
// ---------------------------------------------------------------------------
void run_negative_coord_tests() {
    RegionEnsureFixture f;

    WorldRegionEnsureRequest req;
    req.world_id = "test_world";
    req.world_seed = 42;
    req.generator_version = "1.0.0";
    req.content_version = "1.0.0";
    req.worldgen_profile_key = "first_world";
    req.layer_key = "surface";
    req.region_size = 16;
    req.ensure_kind = WorldRegionEnsureKind::TestOnly;
    req.coverage_mode = WorldRegionCoverageMode::ExactCoords;
    req.target_coords = {WorldCellCoord{-9, 0, "surface"}};
    req.max_regions_to_generate = 1;
    req.allow_generate = true;

    auto res = f.ensure_service.ensureRegions(req);
    assert(res.is_ok());
    assert(res.value().ok);

    // Verify cell (-9,0) exists
    auto cell_res = f.runtime.findCell(WorldCellCoord{-9, 0, "surface"});
    assert(cell_res.is_ok());

    // Verify the region is (-1,0), not (0,0)
    bool found_minus_one = false;
    for (const auto& item : res.value().item_results) {
        if (item.region_key.rx == -1 && item.region_key.ry == 0) {
            found_minus_one = true;
        }
    }
    assert(found_minus_one);

    std::cout << "world_region_ensure_negative_coord: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Bootstrap ensure: actor vision window
// ---------------------------------------------------------------------------
void run_bootstrap_ensure_tests() {
    RegionEnsureFixture f;

    auto res = f.ensure_adapter.ensureForBootstrap("player", "surface");
    assert(res.is_ok());
    assert(res.value().ok);
    // Should ensure at least origin region and possibly neighbors
    assert(res.value().item_results.size() > 0);

    std::cout << "world_region_ensure_bootstrap: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Refresh ensure: vision radius
// ---------------------------------------------------------------------------
void run_refresh_ensure_tests() {
    RegionEnsureFixture f;

    auto res = f.ensure_adapter.ensureForRefresh("player", "surface", 3);
    assert(res.is_ok());
    assert(res.value().ok);

    std::cout << "world_region_ensure_refresh: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Max regions limit
// ---------------------------------------------------------------------------
void run_max_regions_limit_tests() {
    RegionEnsureFixture f;

    WorldRegionEnsureRequest req;
    req.world_id = "test_world";
    req.world_seed = 42;
    req.generator_version = "1.0.0";
    req.content_version = "1.0.0";
    req.worldgen_profile_key = "first_world";
    req.layer_key = "surface";
    req.region_size = 16;
    req.ensure_kind = WorldRegionEnsureKind::TestOnly;
    req.coverage_mode = WorldRegionCoverageMode::RegionList;
    req.explicit_regions = {
        WorldRegionCoord{1, 0},
        WorldRegionCoord{2, 0},
        WorldRegionCoord{3, 0}
    };
    req.max_regions_to_generate = 1;
    req.allow_generate = true;

    auto res = f.ensure_service.ensureRegions(req);
    assert(res.is_ok());
    // Should fail because 3 missing > max 1
    assert(!res.value().ok);
    assert(res.value().failure_reason_keys.size() > 0);

    std::cout << "world_region_ensure_max_limit: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Move guard: ensure target region on move
// ---------------------------------------------------------------------------
void run_move_guard_tests() {
    RegionEnsureFixture f;

    MoveTargetRegionGuard guard(f.ensure_service);

    // Target in ungenerated region (2,0)
    WorldCellCoord target{32, 0, "surface"};
    auto res = guard.ensureMoveTarget("player", target, "test_world", 42, "1.0.0", "1.0.0", "first_world", "surface", 16);
    assert(res.is_ok());
    assert(res.value().ok);
    assert(res.value().generated_region_count == 1);

    // Verify target cell exists
    auto cell_res = f.runtime.findCell(target);
    assert(cell_res.is_ok());

    std::cout << "world_region_ensure_move_guard: passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_ensure_idempotency_tests();
        run_boundary_move_tests();
        run_negative_coord_tests();
        run_bootstrap_ensure_tests();
        run_refresh_ensure_tests();
        run_max_regions_limit_tests();
        run_move_guard_tests();
        return 0;
    }
    std::string test = argv[1];
    if (test == "idempotency") run_ensure_idempotency_tests();
    else if (test == "boundary_move") run_boundary_move_tests();
    else if (test == "negative_coord") run_negative_coord_tests();
    else if (test == "bootstrap") run_bootstrap_ensure_tests();
    else if (test == "refresh") run_refresh_ensure_tests();
    else if (test == "max_limit") run_max_regions_limit_tests();
    else if (test == "move_guard") run_move_guard_tests();
    else {
        std::cerr << "Unknown test: " << test << std::endl;
        return 1;
    }
    return 0;
}
