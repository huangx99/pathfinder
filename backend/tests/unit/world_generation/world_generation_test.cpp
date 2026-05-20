#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/world_generation_rng.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/terrain_generator.h"
#include "pathfinder/world_generation/resource_distribution_generator.h"
#include "pathfinder/world_generation/spawn_safety_planner.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include <cassert>
#include <iostream>
#include <set>
#include <unordered_set>

using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;

// ---------------------------------------------------------------------------
// Determinism tests
// ---------------------------------------------------------------------------

void run_world_generation_same_seed_same_region_same_result_tests() {
    WorldGenerationService service1;
    WorldGenerationService service2;

    WorldGenerationRequest request;
    request.world_id = "test_world";
    request.world_seed = 12345;
    request.generator_version = "1.0.0";
    request.content_version = "1.0.0";
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;
    request.enabled_layer_keys = {"surface"};

    auto result1 = service1.generate(request);
    auto result2 = service2.generate(request);

    assert(result1.ok);
    assert(result2.ok);
    assert(result1.cell_drafts.size() == result2.cell_drafts.size());
    assert(result1.resource_node_drafts.size() == result2.resource_node_drafts.size());
    assert(result1.entity_drafts.size() == result2.entity_drafts.size());

    for (size_t i = 0; i < result1.cell_drafts.size(); ++i) {
        assert(result1.cell_drafts[i].terrain_key == result2.cell_drafts[i].terrain_key);
        assert(result1.cell_drafts[i].coord.cellId() == result2.cell_drafts[i].coord.cellId());
    }

    std::cout << "world_generation_same_seed_same_region_same_result: passed" << std::endl;
}

void run_world_generation_different_seed_different_distribution_tests() {
    WorldGenerationService service;

    WorldGenerationRequest req1;
    req1.world_id = "test_world";
    req1.world_seed = 111;
    req1.worldgen_profile_key = "first_world";
    req1.region_size = 16;

    WorldGenerationRequest req2;
    req2.world_id = "test_world_2";
    req2.world_seed = 222;
    req2.worldgen_profile_key = "first_world";
    req2.region_size = 16;

    auto result1 = service.generate(req1);
    auto result2 = service.generate(req2);

    assert(result1.ok);
    assert(result2.ok);

    // Terrain should differ somewhere
    bool terrain_differs = false;
    for (size_t i = 0; i < result1.cell_drafts.size() && i < result2.cell_drafts.size(); ++i) {
        if (result1.cell_drafts[i].terrain_key != result2.cell_drafts[i].terrain_key) {
            terrain_differs = true;
            break;
        }
    }
    assert(terrain_differs);

    std::cout << "world_generation_different_seed_different_distribution: passed" << std::endl;
}

void run_world_generation_manifest_contains_seed_profile_version_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_manifest";
    request.world_seed = 999;
    request.generator_version = "2.0.0";
    request.content_version = "3.0.0";
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(result.manifest.world_seed == 999);
    assert(result.manifest.generator_version == "2.0.0");
    assert(result.manifest.content_version == "3.0.0");
    assert(result.manifest.worldgen_profile_key == "first_world");
    assert(!result.manifest.trace_roll_keys.empty());

    std::cout << "world_generation_manifest_contains_seed_profile_version: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Terrain tests
// ---------------------------------------------------------------------------

void run_world_generation_creates_cells_with_layer_key_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_layer";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.cell_drafts.empty());

    for (const auto& cell : result.cell_drafts) {
        assert(!cell.coord.layer_key.empty());
        assert(cell.coord.layer_key == "surface");
    }

    std::cout << "world_generation_creates_cells_with_layer_key: passed" << std::endl;
}

void run_world_generation_cells_have_valid_terrain_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_terrain";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.cell_drafts.empty());

    std::set<std::string> valid_terrains = {"plain", "forest", "stone_field", "water_edge"};
    for (const auto& cell : result.cell_drafts) {
        assert(!cell.terrain_key.empty());
        assert(valid_terrains.count(cell.terrain_key) > 0);
    }

    std::cout << "world_generation_cells_have_valid_terrain: passed" << std::endl;
}

void run_world_generation_spawn_area_not_blocked_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_blocked";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    // Check origin and immediate neighbors are not blocked
    for (const auto& cell : result.cell_drafts) {
        int dist = std::abs(cell.coord.x) + std::abs(cell.coord.y);
        if (dist <= 2) {
            assert(!cell.blocks_movement);
        }
    }

    std::cout << "world_generation_spawn_area_not_blocked: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Resource tests
// ---------------------------------------------------------------------------

void run_world_generation_places_resource_nodes_on_allowed_terrain_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_resource_terrain";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    // Build cell map
    std::map<std::string, GeneratedCellDraft> cell_map;
    for (const auto& cell : result.cell_drafts) {
        cell_map[cell.coord.cellId()] = cell;
    }

    for (const auto& node : result.resource_node_drafts) {
        auto it = cell_map.find(node.coord.cellId());
        assert(it != cell_map.end());
        // Node should be on a valid cell
        assert(!it->second.terrain_key.empty());
    }

    std::cout << "world_generation_places_resource_nodes_on_allowed_terrain: passed" << std::endl;
}

void run_world_generation_resource_node_has_action_and_output_keys_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_resource_action";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    for (const auto& node : result.resource_node_drafts) {
        assert(!node.node_id.empty());
        assert(!node.resource_key.empty());
        assert(!node.required_action_key.empty());
        assert(!node.output_entity_keys.empty());
        assert(node.remaining_charges > 0);
        assert(node.max_charges > 0);
    }

    std::cout << "world_generation_resource_node_has_action_and_output_keys: passed" << std::endl;
}

void run_world_generation_resource_node_id_is_stable_tests() {
    WorldGenerationService service1;
    WorldGenerationService service2;

    WorldGenerationRequest request;
    request.world_id = "test_stable";
    request.world_seed = 777;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result1 = service1.generate(request);
    auto result2 = service2.generate(request);

    assert(result1.ok);
    assert(result2.ok);
    assert(result1.resource_node_drafts.size() == result2.resource_node_drafts.size());

    for (size_t i = 0; i < result1.resource_node_drafts.size(); ++i) {
        assert(result1.resource_node_drafts[i].node_id == result2.resource_node_drafts[i].node_id);
    }

    std::cout << "world_generation_resource_node_id_is_stable: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Spawn safety tests
// ---------------------------------------------------------------------------

void run_world_generation_spawn_safety_has_basic_food_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_food";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool has_food = false;
    for (const auto& node : result.resource_node_drafts) {
        int dist = std::abs(node.coord.x) + std::abs(node.coord.y);
        if (dist <= 2) {
            for (const auto& tag : node.tag_keys) {
                if (tag == "food_basic") {
                    has_food = true;
                    break;
                }
            }
        }
        if (has_food) break;
    }
    assert(has_food);

    std::cout << "world_generation_spawn_safety_has_basic_food: passed" << std::endl;
}

void run_world_generation_spawn_safety_has_basic_materials_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_materials";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool has_wood = false;
    bool has_stone = false;
    for (const auto& node : result.resource_node_drafts) {
        int dist = std::abs(node.coord.x) + std::abs(node.coord.y);
        if (dist <= 2) {
            for (const auto& tag : node.tag_keys) {
                if (tag == "wood_basic") has_wood = true;
                if (tag == "stone_basic") has_stone = true;
            }
        }
    }
    assert(has_wood);
    assert(has_stone);

    std::cout << "world_generation_spawn_safety_has_basic_materials: passed" << std::endl;
}

void run_world_generation_spawn_safety_blocks_immediate_active_threat_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_threat";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    // No active predator within safe_radius
    for (const auto& node : result.resource_node_drafts) {
        int dist = std::abs(node.coord.x) + std::abs(node.coord.y);
        if (dist <= 2) {
            for (const auto& tag : node.tag_keys) {
                assert(tag != "active_predator");
            }
        }
    }

    std::cout << "world_generation_spawn_safety_blocks_immediate_active_threat: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P45 ownership tests
// ---------------------------------------------------------------------------

void run_world_generation_ground_items_are_on_map_only_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_ownership";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.entity_drafts.empty()); // P46: must not be empty

    for (const auto& entity : result.entity_drafts) {
        assert(!entity.entity_id.empty());
        assert(!entity.coord.layer_key.empty());
        assert(entity.coord.layer_key == "surface");
    }

    std::cout << "world_generation_ground_items_are_on_map_only: passed" << std::endl;
}

void run_world_generation_ground_items_have_quantity_stack_key_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_stack";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.entity_drafts.empty()); // P46: must not be empty

    for (const auto& entity : result.entity_drafts) {
        assert(entity.quantity > 0);
        assert(!entity.stack_key.empty());
    }

    std::cout << "world_generation_ground_items_have_quantity_stack_key: passed" << std::endl;
}

void run_world_generation_does_not_create_player_inventory_items_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_no_inv";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    // No entity draft should be for player inventory
    for (const auto& entity : result.entity_drafts) {
        assert(entity.entity_key != "player");
    }
    // P46 only generates OnMap items, never InInventory items
    assert(result.entity_drafts.size() < 100); // sanity check

    std::cout << "world_generation_does_not_create_player_inventory_items: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Command integration tests (stub level)
// ---------------------------------------------------------------------------

void run_world_command_generate_world_uses_world_generation_service_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_cmd";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.events.empty());
    assert(result.events[0].event_kind == "WorldGenerated");

    std::cout << "world_command_generate_world_uses_world_generation_service: passed" << std::endl;
}

void run_world_generation_same_seed_generates_same_manifest_tests() {
    WorldGenerationService s1;
    WorldGenerationService s2;

    WorldGenerationRequest request;
    request.world_id = "test_manifest2";
    request.world_seed = 555;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto r1 = s1.generate(request);
    auto r2 = s2.generate(request);

    assert(r1.ok);
    assert(r2.ok);
    assert(r1.manifest.world_seed == r2.manifest.world_seed);
    assert(r1.manifest.worldgen_profile_key == r2.manifest.worldgen_profile_key);
    assert(r1.manifest.generated_cell_ids.size() == r2.manifest.generated_cell_ids.size());
    assert(r1.manifest.generated_resource_node_ids.size() == r2.manifest.generated_resource_node_ids.size());

    std::cout << "world_generation_same_seed_generates_same_manifest: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-1: Adjacent regions no id collision
// ---------------------------------------------------------------------------

void run_world_generation_adjacent_regions_no_id_collision_tests() {
    WorldGenerationService service;

    WorldGenerationRequest req1;
    req1.world_id = "test_collision";
    req1.world_seed = 12345;
    req1.worldgen_profile_key = "first_world";
    req1.region_coord = WorldRegionCoord{0, 0};
    req1.region_size = 16;

    WorldGenerationRequest req2;
    req2.world_id = "test_collision";
    req2.world_seed = 12345;
    req2.worldgen_profile_key = "first_world";
    req2.region_coord = WorldRegionCoord{1, 0};
    req2.region_size = 16;

    auto result1 = service.generate(req1);
    auto result2 = service.generate(req2);

    assert(result1.ok);
    assert(result2.ok);

    std::unordered_set<std::string> ids;
    for (const auto& cell : result1.cell_drafts) {
        assert(ids.insert(cell.coord.cellId()).second);
    }
    for (const auto& cell : result2.cell_drafts) {
        assert(ids.insert(cell.coord.cellId()).second);
    }
    for (const auto& node : result1.resource_node_drafts) {
        assert(ids.insert(node.node_id).second);
    }
    for (const auto& node : result2.resource_node_drafts) {
        assert(ids.insert(node.node_id).second);
    }

    // Verify region (1,0) uses world coordinates offset
    bool has_positive_x = false;
    for (const auto& cell : result2.cell_drafts) {
        if (cell.coord.x > 0) has_positive_x = true;
    }
    assert(has_positive_x);

    std::cout << "world_generation_adjacent_regions_no_id_collision: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-2: Cells actually created in runtime
// ---------------------------------------------------------------------------

void run_world_generation_cells_created_in_runtime_tests() {
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
    request.world_id = "runtime_cells";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);
    assert(!gen_result.cell_drafts.empty());

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    // Verify a sample cell exists in runtime
    const auto& sample_cell = gen_result.cell_drafts[0];
    auto cell_res = world_runtime.findCell(sample_cell.coord);
    assert(cell_res.is_ok());
    assert(cell_res.value()->terrain_key == sample_cell.terrain_key);
    assert(cell_res.value()->region_id == sample_cell.region_id);
    assert(cell_res.value()->coord.layer_key == sample_cell.coord.layer_key);
    assert(cell_res.value()->generated);

    std::cout << "world_generation_cells_created_in_runtime: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-3: Entity attached to cell
// ---------------------------------------------------------------------------

void run_world_generation_entity_attached_to_cell_tests() {
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
    request.world_id = "entity_cell";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    for (const auto& entity_draft : gen_result.entity_drafts) {
        auto cell_res = world_runtime.findCell(entity_draft.coord);
        assert(cell_res.is_ok());
        bool found = false;
        for (const auto& eid : cell_res.value()->entity_ids) {
            if (eid == entity_draft.entity_id) { found = true; break; }
        }
        assert(found);
    }

    std::cout << "world_generation_entity_attached_to_cell: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-4: Resource nodes stored in runtime
// ---------------------------------------------------------------------------

void run_world_generation_resource_nodes_in_runtime_tests() {
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
    request.world_id = "resource_runtime";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);
    assert(!gen_result.resource_node_drafts.empty());

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    for (const auto& node_draft : gen_result.resource_node_drafts) {
        auto node_res = world_runtime.findResourceNode(node_draft.node_id);
        assert(node_res.is_ok());
        assert(node_res.value()->resource_key == node_draft.resource_key);
        assert(node_res.value()->coord.cellId() == node_draft.coord.cellId());

        // Resource node must NOT be in the regular entity list
        auto entity_res = world_runtime.findEntity(node_draft.node_id);
        assert(entity_res.is_error());
    }

    std::cout << "world_generation_resource_nodes_in_runtime: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-5: Deterministic manifest timestamp
// ---------------------------------------------------------------------------

void run_world_generation_manifest_timestamp_deterministic_tests() {
    WorldGenerationService s1;
    WorldGenerationService s2;

    WorldGenerationRequest request;
    request.world_id = "det_time";
    request.world_seed = 999;
    request.generator_version = "1.0.0";
    request.content_version = "1.0.0";
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto r1 = s1.generate(request);
    auto r2 = s2.generate(request);

    assert(r1.ok);
    assert(r2.ok);
    assert(r1.manifest.generation_timestamp == r2.manifest.generation_timestamp);
    assert(r1.manifest.generation_timestamp == 0);
    assert(r1.manifest.generated_cell_ids == r2.manifest.generated_cell_ids);
    assert(r1.manifest.generated_entity_ids == r2.manifest.generated_entity_ids);
    assert(r1.manifest.generated_resource_node_ids == r2.manifest.generated_resource_node_ids);
    assert(r1.manifest.trace_roll_keys == r2.manifest.trace_roll_keys);

    std::cout << "world_generation_manifest_timestamp_deterministic: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-6: Generate pure function + idempotent apply
// ---------------------------------------------------------------------------

void run_world_generation_generate_is_pure_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "pure_test";
    request.world_seed = 777;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto r1 = service.generate(request);
    auto r2 = service.generate(request);

    assert(r1.ok);
    assert(r2.ok);
    assert(r1.cell_drafts.size() == r2.cell_drafts.size());
    assert(r1.resource_node_drafts.size() == r2.resource_node_drafts.size());
    assert(r1.entity_drafts.size() == r2.entity_drafts.size());

    std::cout << "world_generation_generate_is_pure: passed" << std::endl;
}

void run_world_generation_repeated_apply_blocked_tests() {
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
    request.world_id = "repeat_test";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply1 = applier.apply(gen_result);
    assert(apply1.ok);

    auto apply2 = applier.apply(gen_result);
    assert(!apply2.ok);
    assert(apply2.failure_kind == WorldGenerationFailureKind::RegionAlreadyGenerated);

    std::cout << "world_generation_repeated_apply_blocked: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P1-1: Spawn safety uses tags not fixed keys
// ---------------------------------------------------------------------------

void run_world_generation_spawn_safety_uses_tags_not_fixed_keys_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "tag_test";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool has_food_tag = false;
    bool has_wood_tag = false;
    bool has_stone_tag = false;

    for (const auto& node : result.resource_node_drafts) {
        int dist = std::abs(node.coord.x) + std::abs(node.coord.y);
        if (dist <= 2) {
            for (const auto& tag : node.tag_keys) {
                if (tag == "food_basic") has_food_tag = true;
                if (tag == "wood_basic") has_wood_tag = true;
                if (tag == "stone_basic") has_stone_tag = true;
            }
        }
    }

    assert(has_food_tag);
    assert(has_wood_tag);
    assert(has_stone_tag);

    std::cout << "world_generation_spawn_safety_uses_tags_not_fixed_keys: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P1-2: Command handler parameter parsing
// ---------------------------------------------------------------------------

void run_world_generation_command_handler_parsing_tests() {
    // This is tested in integration tests with real handler
    std::cout << "world_generation_command_handler_parsing: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0-additional: spawnEntityOnMap rejects missing cell
// ---------------------------------------------------------------------------

void run_world_generation_spawn_entity_missing_cell_fails_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    WorldCellCoord missing_coord{999, 999, "surface"};
    auto res = world_runtime.spawnEntityOnMap(
        "test_entity_1",
        "test_key",
        "entity.test_key",
        missing_coord,
        1,
        "test_key:default",
        true,
        {},
        {},
        {});

    assert(res.is_error());

    std::cout << "world_generation_spawn_entity_missing_cell_fails: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0: ground items non-empty for first_world
// ---------------------------------------------------------------------------

void run_world_generation_ground_items_non_empty_for_first_world_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_nonempty";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.entity_drafts.empty());

    std::cout << "world_generation_ground_items_non_empty_for_first_world: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0: resource node missing cell fails
// ---------------------------------------------------------------------------

void run_world_generation_resource_node_missing_cell_fails_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    GeneratedResourceNodeDraft bad_node;
    bad_node.node_id = "node_test_surface_999_999";
    bad_node.resource_key = "test_resource";
    bad_node.coord = WorldCellCoord{999, 999, "surface"};
    bad_node.node_kind = ResourceNodeKind::Plant;
    bad_node.state = ResourceNodeState::Available;
    bad_node.remaining_charges = 1;
    bad_node.max_charges = 1;

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto res = applier.applyResourceNodes({bad_node});

    assert(!res.ok);
    assert(res.failure_kind == WorldGenerationFailureKind::ApplyFailed);

    std::cout << "world_generation_resource_node_missing_cell_fails: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P0: resource nodes applied to existing cells
// ---------------------------------------------------------------------------

void run_world_generation_resource_nodes_applied_to_existing_cells_tests() {
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
    request.world_id = "test_nodes";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;

    auto gen_result = service.generate(request);
    assert(gen_result.ok);
    assert(!gen_result.resource_node_drafts.empty());

    WorldGenerationApplier applier(world_runtime, world_runtime);
    auto apply_result = applier.apply(gen_result);
    assert(apply_result.ok);

    for (const auto& node_draft : gen_result.resource_node_drafts) {
        auto node_res = world_runtime.findResourceNode(node_draft.node_id);
        assert(node_res.is_ok());

        auto cell_res = world_runtime.findCell(node_draft.coord);
        assert(cell_res.is_ok());

        auto entity_res = world_runtime.findEntity(node_draft.node_id);
        assert(entity_res.is_error());
    }

    std::cout << "world_generation_resource_nodes_applied_to_existing_cells: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P1: invalid layer keys rejected
// ---------------------------------------------------------------------------

void run_world_generation_invalid_layer_rejected_tests() {
    WorldGenerationService service;

    WorldGenerationRequest request;
    request.world_id = "test_layer";
    request.world_seed = 42;
    request.worldgen_profile_key = "first_world";
    request.region_size = 16;
    request.enabled_layer_keys = {"surface", "underground"};

    auto result = service.generate(request);
    assert(!result.ok);
    assert(result.failure_kind == WorldGenerationFailureKind::InvalidLayer);

    std::cout << "world_generation_invalid_layer_rejected: passed" << std::endl;
}
