#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_command/world_command_registry.h"
#include "pathfinder/world_command/world_command_dispatcher.h"
#include "pathfinder/world_command/world_command_pipeline.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;

void run_generate_world_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(pathfinder::world_runtime::createGenerateWorldCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createWaitCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createMoveCommandHandler(runtime, nullptr));
    registry.registerHandler(pathfinder::world_runtime::createInspectCommandHandler(runtime));

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    WorldCommandDto cmd;
    cmd.command_id = "cmd_gen";
    cmd.command_key = "generate_world";
    cmd.command_kind = WorldCommandKind::GenerateWorld;
    cmd.source = WorldCommandSource::PlayerInput;
    cmd.actor_key = "player";
    cmd.parameters["seed"] = "42";
    cmd.parameters["region_size"] = "9";
    cmd.parameters["vision_radius"] = "3";

    auto result = pipeline.execute("session_1", cmd);
    assert(result.is_ok());
    auto response = result.value();
    assert(response.result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!response.event_feed.empty());
    assert(response.event_feed[0].event_kind == "WorldGenerated");

    // Verify runtime state
    assert(runtime.currentWorldTick() == 0);
    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->coord.x == 0);
    assert(actor_res.value()->coord.y == 0);

    std::cout << "world_command_generate_world_runtime: passed" << std::endl;
}

void run_move_runtime_tests() {
    WorldGridRuntime runtime;

    // Setup world first via bridge
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "12345";
    gen_cmd.parameters["region_size"] = "9";
    bridge.handleGenerateWorld(ctx, gen_cmd);

    // Find a valid adjacent target
    WorldCellCoord target{1, 0, "surface"};
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{0, 1, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{-1, 0, "surface"};
    }

    WorldCommandDto move_cmd;
    move_cmd.command_id = "cmd_move";
    move_cmd.command_key = "move";
    move_cmd.command_kind = WorldCommandKind::Move;
    move_cmd.source = WorldCommandSource::PlayerInput;
    move_cmd.actor_key = "player";
    move_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_cmd.target.target_coord = WorldCoordinateDto{target.x, target.y, target.layer_key};

    auto result = bridge.handleMove(ctx, move_cmd);
    assert(result.is_ok());
    auto exec_result = result.value();
    assert(exec_result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!exec_result.events.empty());
    assert(exec_result.events[0].event_kind == "ActorMoved");

    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    assert(actor_res.value()->coord == target);

    std::cout << "world_command_move_runtime: passed" << std::endl;
}

void run_move_blocked_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "42";
    gen_cmd.parameters["region_size"] = "9";
    bridge.handleGenerateWorld(ctx, gen_cmd);

    // Try to move far away
    WorldCommandDto move_cmd;
    move_cmd.command_id = "cmd_move";
    move_cmd.command_key = "move";
    move_cmd.command_kind = WorldCommandKind::Move;
    move_cmd.source = WorldCommandSource::PlayerInput;
    move_cmd.actor_key = "player";
    move_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_cmd.target.target_coord = WorldCoordinateDto{10, 10, "surface"};

    auto result = bridge.handleMove(ctx, move_cmd);
    assert(result.is_ok());
    auto exec_result = result.value();
    assert(exec_result.result_kind == WorldCommandResultKind::Blocked);
    assert(!exec_result.failure_reason_keys.empty());

    std::cout << "world_command_move_blocked_runtime: passed" << std::endl;
}

void run_inspect_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "42";
    gen_cmd.parameters["region_size"] = "9";
    bridge.handleGenerateWorld(ctx, gen_cmd);

    WorldCommandDto inspect_cmd;
    inspect_cmd.command_id = "cmd_inspect";
    inspect_cmd.command_key = "inspect";
    inspect_cmd.command_kind = WorldCommandKind::Inspect;
    inspect_cmd.source = WorldCommandSource::PlayerInput;
    inspect_cmd.actor_key = "player";
    inspect_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    inspect_cmd.target.target_coord = WorldCoordinateDto{0, 0, "surface"};

    auto result = bridge.handleInspect(ctx, inspect_cmd);
    assert(result.is_ok());
    auto exec_result = result.value();
    assert(exec_result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!exec_result.events.empty());
    assert(exec_result.events[0].event_kind == "Inspect");

    std::cout << "world_command_inspect_runtime: passed" << std::endl;
}

void run_projection_patch_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "12345";
    gen_cmd.parameters["region_size"] = "9";
    bridge.handleGenerateWorld(ctx, gen_cmd);

    // Move to generate changes
    WorldCellCoord target{1, 0, "surface"};
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{0, 1, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{-1, 0, "surface"};
    }

    WorldCommandDto move_cmd;
    move_cmd.command_id = "cmd_move";
    move_cmd.command_key = "move";
    move_cmd.source = WorldCommandSource::PlayerInput;
    move_cmd.actor_key = "player";
    move_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_cmd.target.target_coord = WorldCoordinateDto{target.x, target.y, target.layer_key};

    auto move_result = bridge.handleMove(ctx, move_cmd);
    assert(move_result.is_ok());

    // Build projection patch
    std::vector<std::string> changed_cells = {"surface:0:0", target.cellId()};
    std::vector<std::string> changed_entities;
    auto actor_res = runtime.findActor("player");
    if (actor_res.is_ok()) {
        changed_entities.push_back(actor_res.value()->entity_id);
    }

    auto patch = bridge.buildProjectionPatch(changed_cells, changed_entities, "player", false);
    assert(patch.new_projection_version > 0);
    assert(!patch.changed_cells.empty());
    assert(!patch.changed_entities.empty());

    std::cout << "world_command_projection_patch_runtime: passed" << std::endl;
}

void run_pipeline_with_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(pathfinder::world_runtime::createGenerateWorldCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createWaitCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createMoveCommandHandler(runtime, nullptr));
    registry.registerHandler(pathfinder::world_runtime::createInspectCommandHandler(runtime));

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Generate world
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.command_kind = WorldCommandKind::GenerateWorld;
    gen_cmd.source = WorldCommandSource::PlayerInput;
    gen_cmd.actor_key = "player";
    gen_cmd.parameters["seed"] = "42";
    gen_cmd.parameters["region_size"] = "9";

    auto gen_result = pipeline.execute("session_1", gen_cmd);
    assert(gen_result.is_ok());
    assert(gen_result.value().result.result_kind == WorldCommandResultKind::Succeeded);

    // Move
    WorldCellCoord target{1, 0, "surface"};
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{0, 1, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{-1, 0, "surface"};
    }

    WorldCommandDto move_cmd;
    move_cmd.command_id = "cmd_move";
    move_cmd.command_key = "move";
    move_cmd.command_kind = WorldCommandKind::Move;
    move_cmd.source = WorldCommandSource::PlayerInput;
    move_cmd.actor_key = "player";
    move_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_cmd.target.target_coord = WorldCoordinateDto{target.x, target.y, target.layer_key};

    auto move_result = pipeline.execute("session_1", move_cmd);
    assert(move_result.is_ok());
    assert(move_result.value().result.result_kind == WorldCommandResultKind::Succeeded);

    // Wait
    WorldCommandDto wait_cmd;
    wait_cmd.command_id = "cmd_wait";
    wait_cmd.command_key = "wait";
    wait_cmd.command_kind = WorldCommandKind::Wait;
    wait_cmd.source = WorldCommandSource::PlayerInput;
    wait_cmd.actor_key = "player";

    auto wait_result = pipeline.execute("session_1", wait_cmd);
    assert(wait_result.is_ok());
    assert(wait_result.value().result.result_kind == WorldCommandResultKind::Succeeded);
    assert(runtime.currentWorldTick() > 0);

    std::cout << "world_command_pipeline_with_runtime: passed" << std::endl;
}

void run_no_playbook_rules_tests() {
    // Verify that runtime core does not contain hardcoded gameplay keys
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;

    WorldGridRuntime runtime;
    runtime.initialize(config);
    runtime.generateInitialWorld(config);

    auto snap = runtime.snapshotForDebug();
    assert(snap.is_ok());

    for (const auto& [id, cell] : snap.value().cells) {
        assert(cell.terrain_key != "red_berry");
        assert(cell.terrain_key != "wolf");
        assert(cell.terrain_key != "torch");
        assert(cell.terrain_key != "axe");
    }

    for (const auto& [id, entity] : snap.value().entities) {
        assert(entity.entity_key != "red_berry");
        assert(entity.entity_key != "wolf");
        assert(entity.entity_key != "torch");
        assert(entity.entity_key != "axe");
    }

    std::cout << "world_runtime_no_playbook_rules: passed" << std::endl;
}

void run_wait_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "42";
    bridge.handleGenerateWorld(ctx, gen_cmd);

    uint64_t old_tick = runtime.currentWorldTick();

    WorldCommandDto wait_cmd;
    wait_cmd.command_id = "cmd_wait";
    wait_cmd.command_key = "wait";
    wait_cmd.source = WorldCommandSource::PlayerInput;
    wait_cmd.actor_key = "player";
    wait_cmd.parameters["tick_delta"] = "3";

    auto result = bridge.handleWait(ctx, wait_cmd);
    assert(result.is_ok());
    auto exec_result = result.value();
    assert(exec_result.result_kind == WorldCommandResultKind::Succeeded);
    assert(!exec_result.events.empty());
    assert(exec_result.events[0].event_kind == "TimePassed");
    assert(runtime.currentWorldTick() == old_tick + 3);

    std::cout << "world_command_wait_runtime: passed" << std::endl;
}

void run_pipeline_projection_patch_runtime_tests() {
    WorldGridRuntime runtime;
    WorldCommandHandlerRegistry registry;
    registry.registerHandler(pathfinder::world_runtime::createGenerateWorldCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createWaitCommandHandler(runtime));
    registry.registerHandler(pathfinder::world_runtime::createMoveCommandHandler(runtime, nullptr));
    registry.registerHandler(pathfinder::world_runtime::createInspectCommandHandler(runtime));

    WorldCommandDispatcher dispatcher(registry);
    WorldCommandPipeline pipeline(dispatcher);

    // Generate world
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.command_kind = WorldCommandKind::GenerateWorld;
    gen_cmd.source = WorldCommandSource::PlayerInput;
    gen_cmd.actor_key = "player";
    gen_cmd.parameters["seed"] = "42";
    gen_cmd.parameters["region_size"] = "9";

    auto gen_result = pipeline.execute("session_1", gen_cmd);
    assert(gen_result.is_ok());
    auto gen_response = gen_result.value();
    assert(gen_response.result.result_kind == WorldCommandResultKind::Succeeded);
    // GenerateWorld should return full refresh with changed_cells
    assert(gen_response.projection_patch.requires_full_refresh);
    assert(!gen_response.projection_patch.changed_cells.empty());
    assert(!gen_response.projection_patch.changed_entities.empty());

    // Verify cells have x/y/layer_key
    for (const auto& cell : gen_response.projection_patch.changed_cells) {
        assert(cell.fields.count("x"));
        assert(cell.fields.count("y"));
        assert(cell.fields.count("layer_key"));
    }

    // Move
    WorldCellCoord target{1, 0, "surface"};
    auto cell_res = runtime.findCell(target);
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{0, 1, "surface"};
        cell_res = runtime.findCell(target);
    }
    if (cell_res.is_ok() && cell_res.value()->blocks_movement) {
        target = WorldCellCoord{-1, 0, "surface"};
    }

    WorldCommandDto move_cmd;
    move_cmd.command_id = "cmd_move";
    move_cmd.command_key = "move";
    move_cmd.command_kind = WorldCommandKind::Move;
    move_cmd.source = WorldCommandSource::PlayerInput;
    move_cmd.actor_key = "player";
    move_cmd.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_cmd.target.target_coord = WorldCoordinateDto{target.x, target.y, target.layer_key};

    auto move_result = pipeline.execute("session_1", move_cmd);
    assert(move_result.is_ok());
    auto move_response = move_result.value();
    assert(move_response.result.result_kind == WorldCommandResultKind::Succeeded);
    // Move should return changed_cells and changed_entities via Pipeline
    assert(!move_response.projection_patch.changed_cells.empty());
    assert(!move_response.projection_patch.changed_entities.empty());

    // Verify player entity is in changed_entities
    auto actor_res = runtime.findActor("player");
    assert(actor_res.is_ok());
    std::string player_entity_id = actor_res.value()->entity_id;
    bool found_player_entity = false;
    for (const auto& ep : move_response.projection_patch.changed_entities) {
        if (ep.entity_id == player_entity_id) {
            found_player_entity = true;
            assert(ep.fields.count("x"));
            assert(ep.fields.count("y"));
            assert(ep.fields.count("layer_key"));
            break;
        }
    }
    assert(found_player_entity);

    std::cout << "world_command_pipeline_projection_patch_runtime: passed" << std::endl;
}

void run_invalid_parameters_tests() {
    WorldGridRuntime runtime;
    WorldCommandRuntimeBridge bridge(runtime);
    WorldCommandContext ctx(WorldCommandDto{});

    // Invalid seed parameter
    WorldCommandDto gen_cmd;
    gen_cmd.command_id = "cmd_gen";
    gen_cmd.command_key = "generate_world";
    gen_cmd.source = WorldCommandSource::WorldGeneration;
    gen_cmd.parameters["seed"] = "not_a_number";
    auto result = bridge.handleGenerateWorld(ctx, gen_cmd);
    assert(result.is_ok()); // should not crash, use default
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);

    // Invalid tick_delta parameter
    WorldCommandDto wait_cmd;
    wait_cmd.command_id = "cmd_wait";
    wait_cmd.command_key = "wait";
    wait_cmd.source = WorldCommandSource::PlayerInput;
    wait_cmd.actor_key = "player";
    wait_cmd.parameters["tick_delta"] = "invalid";
    auto result2 = bridge.handleWait(ctx, wait_cmd);
    assert(result2.is_ok()); // should not crash, use default 1
    assert(result2.value().result_kind == WorldCommandResultKind::Succeeded);

    std::cout << "world_command_runtime_invalid_parameters: passed" << std::endl;
}
