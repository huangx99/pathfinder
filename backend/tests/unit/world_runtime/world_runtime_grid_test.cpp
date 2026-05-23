#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace pathfinder::world_runtime;
using pathfinder::foundation::Result;

void run_generate_deterministic_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime runtime1;
    runtime1.initialize(config);
    runtime1.generateInitialWorld(config);

    WorldGridRuntime runtime2;
    runtime2.initialize(config);
    runtime2.generateInitialWorld(config);

    auto snap1 = runtime1.snapshotForDebug();
    auto snap2 = runtime2.snapshotForDebug();
    assert(snap1.is_ok());
    assert(snap2.is_ok());

    assert(snap1.value().cells.size() == snap2.value().cells.size());
    assert(snap1.value().entities.size() == snap2.value().entities.size());
    assert(snap1.value().actors.size() == snap2.value().actors.size());

    // Check same terrain at same positions
    for (const auto& [cell_id, cell1] : snap1.value().cells) {
        auto it = snap2.value().cells.find(cell_id);
        assert(it != snap2.value().cells.end());
        assert(cell1.terrain_key == it->second.terrain_key);
    }

    std::cout << "world_runtime_generate_deterministic: passed" << std::endl;
}

void run_find_actor_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->actor_key == "player");
    assert(actor_res.value()->coord.x == 0);
    assert(actor_res.value()->coord.y == 0);
    assert(actor_res.value()->coord.layer_key == "surface");
    assert(actor_res.value()->is_player_controlled);

    auto missing = runtime.findActor("nonexistent");
    assert(missing.is_error());

    std::cout << "world_runtime_find_actor: passed" << std::endl;
}

void run_move_adjacent_tests() {
    WorldRuntimeConfig config;
    config.seed = 12345;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    // Find a plain cell adjacent to origin
    WorldCellCoord target{1, 0, "surface"};
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        // Try another direction if blocked
        target = WorldCellCoord{0, 1, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{-1, 0, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{0, -1, "surface"};
    }

    auto move_res = runtime.moveActor("player", target);
    assert(move_res.is_ok());
    auto move_result = move_res.value();
    assert(move_result.moved);
    assert(move_result.to == target);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->coord == target);

    std::cout << "world_runtime_move_adjacent: passed" << std::endl;
}

void run_move_blocked_tests() {
    WorldRuntimeConfig config;
    config.seed = 1; // seed=1 has water at (0,1) which is adjacent to origin
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    // With seed=1, (0,1) is water and blocks movement
    WorldCellCoord blocked{0, 1, "surface"};
    auto cell_res = runtime.findCell(blocked);
    assert(cell_res.is_ok());
    assert(cell_res.value()->blocks_movement);

    // Try to move from origin into blocked cell
    auto move_res = runtime.moveActor("player", blocked);
    assert(move_res.is_ok());
    auto move_result = move_res.value();
    assert(!move_result.moved);
    assert(move_result.block_reason == WorldMoveBlockReason::CellBlocked);

    std::cout << "world_runtime_move_blocked: passed" << std::endl;
}

void run_move_not_adjacent_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCellCoord far{3, 3, "surface"};
    auto move_res = runtime.moveActor("player", far);
    assert(move_res.is_ok());
    auto move_result = move_res.value();
    assert(!move_result.moved);
    assert(move_result.block_reason == WorldMoveBlockReason::NotAdjacent);

    std::cout << "world_runtime_move_not_adjacent: passed" << std::endl;
}

void run_move_out_of_bounds_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 5;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCellCoord out_of_bounds{100, 100, "surface"};
    auto move_res = runtime.moveActor("player", out_of_bounds);
    assert(move_res.is_ok());
    auto move_result = move_res.value();
    assert(!move_result.moved);
    assert(move_result.block_reason == WorldMoveBlockReason::OutOfBounds);

    std::cout << "world_runtime_move_out_of_bounds: passed" << std::endl;
}

void run_visibility_update_tests() {
    WorldRuntimeConfig config;
    config.seed = 1; // known terrain layout
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 1; // small vision to test discovery

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    // Origin should be visible
    assert(runtime.getCellVisibility("player", "surface:0:0") == WorldCellVisibility::Visible);

    // Outside vision should be unknown initially
    assert(runtime.getCellVisibility("player", "surface:5:5") == WorldCellVisibility::Unknown);

    // With seed=1, (1,0) is forest (passable), (2,0) is plain (passable)
    WorldCellCoord step1{1, 0, "surface"};
    auto cell_res = runtime.findCell(step1);
    assert(cell_res.is_ok());
    assert(!cell_res.value()->blocks_movement);

    runtime.moveActor("player", step1);

    // Origin should now be discovered (vision_radius=1, distance from (1,0) to (0,0) is 1, still visible)
    // Move further to (2,0)
    WorldCellCoord step2{2, 0, "surface"};
    auto cell_res2 = runtime.findCell(step2);
    assert(cell_res2.is_ok());
    assert(!cell_res2.value()->blocks_movement);

    auto move2_res = runtime.moveActor("player", step2);
    assert(move2_res.is_ok());
    assert(std::find(move2_res.value().changed_cell_ids.begin(),
        move2_res.value().changed_cell_ids.end(),
        "surface:0:0") != move2_res.value().changed_cell_ids.end());

    // Now origin (0,0) should be Discovered (distance=2 > vision_radius=1)
    assert(runtime.getCellVisibility("player", "surface:0:0") == WorldCellVisibility::Discovered);

    // New position should be visible
    assert(runtime.getCellVisibility("player", "surface:2:0") == WorldCellVisibility::Visible);

    std::cout << "world_runtime_visibility_update: passed" << std::endl;
}

void run_time_step_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;

    WorldGridRuntime runtime;
    runtime.initialize(config);

    assert(runtime.currentWorldTick() == 0);
    uint64_t old_version = runtime.stateVersion();

    auto res = runtime.advanceWorldTime(5);
    assert(res.is_ok());
    auto result = res.value();
    assert(result.previous_tick == 0);
    assert(result.new_tick == 5);
    assert(result.new_state_version > old_version);
    assert(runtime.currentWorldTick() == 5);

    std::cout << "world_runtime_time_step: passed" << std::endl;
}

void run_actor_missing_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    WorldCellCoord target{1, 0, "surface"};
    auto move_res = runtime.moveActor("nonexistent_actor", target);
    assert(move_res.is_ok());
    auto move_result = move_res.value();
    assert(!move_result.moved);
    assert(move_result.block_reason == WorldMoveBlockReason::ActorMissing);

    std::cout << "world_runtime_actor_missing: passed" << std::endl;
}

void run_inspect_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    // Inspect visible cell
    pathfinder::world_command::WorldCommandTargetDto target;
    target.target_kind = pathfinder::world_command::WorldCommandTargetKind::Coordinate;
    pathfinder::world_command::WorldCoordinateDto coord{0, 0, "surface"};
    target.target_coord = coord;

    auto inspect_res = runtime.inspect("player", target);
    assert(inspect_res.is_ok());
    auto inspect_result = inspect_res.value();
    assert(inspect_result.found);
    assert(!inspect_result.inspected_cell_id.empty());

    // Inspect unknown cell should fail
    pathfinder::world_command::WorldCoordinateDto far_coord{10, 10, "surface"};
    target.target_coord = far_coord;
    auto inspect_res2 = runtime.inspect("player", target);
    assert(inspect_res2.is_ok());
    assert(!inspect_res2.value().found);

    std::cout << "world_runtime_inspect: passed" << std::endl;
}

void run_entity_location_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    auto snap = runtime.snapshotForDebug();
    assert(snap.is_ok());

    bool found_entity_on_map = false;
    for (const auto& [id, entity] : snap.value().entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap) {
            found_entity_on_map = true;
            assert(entity.coord.has_value());
            assert(!entity.entity_id.empty());
            assert(!entity.entity_key.empty());
            assert(entity.entity_id != entity.entity_key); // entity_id is instance id, not key
        }
    }

    std::cout << "world_runtime_entity_location: passed" << std::endl;
}

void run_deterministic_entity_ids_tests() {
    WorldRuntimeConfig config;
    config.seed = 123;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime1;
    runtime1.initialize(config);
    runtime1.generateInitialWorld(config);

    WorldGridRuntime runtime2;
    runtime2.initialize(config);
    runtime2.generateInitialWorld(config);

    auto snap1 = runtime1.snapshotForDebug();
    auto snap2 = runtime2.snapshotForDebug();
    assert(snap1.is_ok());
    assert(snap2.is_ok());

    // Entity IDs must match exactly between same-seed runs
    assert(snap1.value().entities.size() == snap2.value().entities.size());
    for (const auto& [id, entity1] : snap1.value().entities) {
        auto it = snap2.value().entities.find(id);
        assert(it != snap2.value().entities.end());
        assert(entity1.entity_key == it->second.entity_key);
        assert(entity1.coord == it->second.coord);
    }

    // Player entity_id must match
    auto actor1 = runtime1.findActor("player");
    auto actor2 = runtime2.findActor("player");
    assert(actor1.is_ok());
    assert(actor2.is_ok());
    assert(actor1.value()->entity_id == actor2.value()->entity_id);
    assert(actor1.value()->entity_id == "player_surface_0_0");

    std::cout << "world_runtime_deterministic_entity_ids: passed" << std::endl;
}

void run_origin_contains_player_entity_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    std::string player_entity_id = actor_res.value()->entity_id;

    auto cell_res = runtime.findCell(WorldCellCoord{0, 0, "surface"});
    assert(cell_res.is_ok());

    bool found_player = false;
    for (const auto& eid : cell_res.value()->entity_ids) {
        if (eid == player_entity_id) {
            found_player = true;
            break;
        }
    }
    assert(found_player);

    std::cout << "world_runtime_origin_contains_player_entity: passed" << std::endl;
}
