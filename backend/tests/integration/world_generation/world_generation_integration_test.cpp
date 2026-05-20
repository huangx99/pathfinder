#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_generation_command_handler.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_command/world_command_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::world_command;

// ---------------------------------------------------------------------------
// Integration: Generate + Apply to WorldGridRuntime
// ---------------------------------------------------------------------------

void run_world_generation_apply_to_runtime_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    WorldGenerationService service;
    WorldGenerationRequest request;
    request.world_id = "int_world";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    assert(!apply_result.changed_cell_ids.empty());
    assert(!apply_result.state_deltas.empty());

    std::cout << "world_generation_apply_to_runtime: passed" << std::endl;
}

void run_world_generation_ground_items_on_map_after_apply_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    WorldGenerationService service;
    WorldGenerationRequest request;
    request.world_id = "int_world";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    // Verify all generated ground items have OnMap coordinates
    for (const auto& entity_draft : gen_result.entity_drafts) {
        auto entity_res = world_runtime.findEntity(entity_draft.entity_id);
        assert(entity_res.is_ok());
        assert(entity_res.value()->coord.has_value());
        assert(entity_res.value()->location_kind == WorldEntityLocationKind::OnMap);
    }

    std::cout << "world_generation_ground_items_on_map_after_apply: passed" << std::endl;
}

void run_world_generation_trace_replayable_tests() {
    WorldGenerationService service;
    WorldGenerationRequest request;
    request.world_id = "trace_world";
    request.world_seed = 123;
    request.generator_version = "1.0.0";
    request.content_version = "1.0.0";
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.manifest.trace_roll_keys.empty());

    // Trace should contain terrain rolls
    bool has_terrain_roll = false;
    for (const auto& key : result.manifest.trace_roll_keys) {
        if (key.find("terrain_") == 0) {
            has_terrain_roll = true;
            break;
        }
    }
    assert(has_terrain_roll);

    std::cout << "world_generation_trace_replayable: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P1-2: Command handler parameter parsing robustness
// ---------------------------------------------------------------------------

void run_world_generation_command_handler_invalid_seed_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    auto service = std::make_shared<WorldGenerationService>();
    auto handler = createGenerateWorldCommandHandler(service, world_runtime, world_runtime);

    WorldCommandDto command;
    command.command_id = "cmd_1";
    command.command_kind = WorldCommandKind::GenerateWorld;
    command.parameters["world_seed"] = "not_a_number";
    command.parameters["worldgen_profile_key"] = "first_world";

    WorldCommandContext context(command);
    auto result = handler->execute(context, command);

    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Failed);
    bool has_invalid = false;
    for (const auto& reason : result.value().failure_reason_keys) {
        if (reason.find("InvalidRequest") != std::string::npos) has_invalid = true;
    }
    assert(has_invalid);

    std::cout << "world_generation_command_handler_invalid_seed: passed" << std::endl;
}

void run_world_generation_command_handler_region_xy_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    auto service = std::make_shared<WorldGenerationService>();
    auto handler = createGenerateWorldCommandHandler(service, world_runtime, world_runtime);

    WorldCommandDto command;
    command.command_id = "cmd_2";
    command.command_kind = WorldCommandKind::GenerateWorld;
    command.parameters["world_seed"] = "42";
    command.parameters["worldgen_profile_key"] = "first_world";
    command.parameters["region_x"] = "1";
    command.parameters["region_y"] = "-1";
    command.parameters["layer_keys"] = "surface";

    WorldCommandContext context(command);
    auto result = handler->execute(context, command);

    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);
    assert(!result.value().changed_cell_ids.empty());

    // Verify coordinates are in region (1,-1): x in [8,23], y in [-24,-9]
    bool has_correct_region = false;
    for (const auto& cell_id : result.value().changed_cell_ids) {
        // cell_id format: layer:x:y, parse x and y
        size_t first_colon = cell_id.find(':');
        if (first_colon != std::string::npos) {
            size_t second_colon = cell_id.find(':', first_colon + 1);
            if (second_colon != std::string::npos) {
                int x = std::stoi(cell_id.substr(first_colon + 1, second_colon - first_colon - 1));
                int y = std::stoi(cell_id.substr(second_colon + 1));
                if (x >= 8 && x <= 23 && y >= -24 && y <= -9) {
                    has_correct_region = true;
                }
            }
        }
    }
    assert(has_correct_region);

    std::cout << "world_generation_command_handler_region_xy: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P1-3: Real command handler integration test
// ---------------------------------------------------------------------------

void run_world_generation_command_handler_integration_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    auto service = std::make_shared<WorldGenerationService>();
    auto handler = createGenerateWorldCommandHandler(service, world_runtime, world_runtime);

    WorldCommandDto command;
    command.command_id = "cmd_3";
    command.command_kind = WorldCommandKind::GenerateWorld;
    command.parameters["world_seed"] = "42";
    command.parameters["worldgen_profile_key"] = "first_world";
    command.parameters["region_size"] = "16";

    WorldCommandContext context(command);
    auto result = handler->execute(context, command);

    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);
    assert(!result.value().changed_cell_ids.empty());
    assert(!result.value().events.empty());

    // Verify runtime has cells
    auto snapshot_res = world_runtime.snapshotForDebug();
    assert(snapshot_res.is_ok());
    const auto& snapshot = snapshot_res.value();
    assert(!snapshot.cells.empty());

    // Verify runtime has resource nodes
    bool has_resource_nodes = false;
    for (const auto& [id, node] : snapshot.resource_nodes) {
        has_resource_nodes = true;
        assert(!node.node_id.empty());
        assert(!node.resource_key.empty());
    }
    assert(has_resource_nodes);

    // Verify entities are on map
    for (const auto& [id, entity] : snapshot.entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap) {
            assert(entity.coord.has_value());
        }
    }

    std::cout << "world_generation_command_handler_integration: passed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests: smoke, apply_to_runtime, ground_items_on_map, trace_replayable" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "smoke") {
        std::cout << "world_generation integration smoke test passed" << std::endl;
        return 0;
    } else if (test_name == "apply_to_runtime") {
        run_world_generation_apply_to_runtime_tests();
    } else if (test_name == "ground_items_on_map") {
        run_world_generation_ground_items_on_map_after_apply_tests();
    } else if (test_name == "trace_replayable") {
        run_world_generation_trace_replayable_tests();
    } else if (test_name == "command_handler_invalid_seed") {
        run_world_generation_command_handler_invalid_seed_tests();
    } else if (test_name == "command_handler_region_xy") {
        run_world_generation_command_handler_region_xy_tests();
    } else if (test_name == "command_handler_integration") {
        run_world_generation_command_handler_integration_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
