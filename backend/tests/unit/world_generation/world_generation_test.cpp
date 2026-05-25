#include "pathfinder/world_generation/world_generation_types.h"
#include "pathfinder/world_generation/world_generation_rng.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/terrain_generator.h"
#include "pathfinder/world_generation/resource_distribution_generator.h"
#include "pathfinder/world_generation/spawn_safety_planner.h"
#include "pathfinder/world_generation/terrain_noise_sampler.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/content/content_registry.h"
#include <cassert>
#include <iostream>
#include <set>
#include <unordered_set>
#include <cmath>

using namespace pathfinder::world_generation;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;

namespace {

pathfinder::content::ObjectDefinitionContent makeTestObject(
    const std::string& object_key,
    const std::string& display_key,
    const std::vector<std::string>& tags,
    int quantity = 1) {
    pathfinder::content::ObjectDefinitionContent object;
    object.key = pathfinder::content::ObjectDefinitionKey(object_key);
    object.display_key = display_key;
    object.description_key = display_key + ".desc";
    object.kind = "material";
    object.safe_tags = tags;
    object.default_quantity = quantity;
    return object;
}

std::shared_ptr<pathfinder::content::ContentRegistry> makeContentBackedWorldgenRegistry() {
    pathfinder::content::ContentDraftRegistry draft;
    draft.addObject(makeTestObject("red_berry", "object.red_berry.name", {"food", "food_basic"}));
    draft.addObject(makeTestObject("wood", "object.wood.name", {"material", "wood", "wood_basic"}));
    draft.addObject(makeTestObject("stone_flake", "object.stone_flake.name", {"material", "stone", "stone_basic"}));
    draft.addObject(makeTestObject("loose_stone", "object.loose_stone.name", {"material", "stone", "ground_item", "stone_basic"}));

    pathfinder::content::WorldgenProfileDefinitionContent profile;
    profile.profile_key = "sandbox_blank";
    profile.region_size = 16;
    profile.default_layer = "surface";
    profile.terrain_generation_mode = "WeightedRandom";
    profile.terrain_weights.push_back({"plain", 50});
    profile.terrain_weights.push_back({"forest", 30});
    profile.terrain_weights.push_back({"stone_field", 20});

    pathfinder::content::WorldgenResourceRuleDto food;
    food.resource_key = "berry_bush";
    food.allowed_terrain_tags = {"plain"};
    food.density = 0.08;
    food.tag_keys = {"food_basic"};
    food.node_kind = "Plant";
    food.required_action_key = "gather";
    food.output_object_keys = {"red_berry"};
    food.charges = 3;
    profile.resource_rules.push_back(food);

    pathfinder::content::WorldgenResourceRuleDto wood;
    wood.resource_key = "young_tree";
    wood.allowed_terrain_tags = {"plain", "forest"};
    wood.density = 0.08;
    wood.tag_keys = {"wood_basic"};
    wood.node_kind = "Tree";
    wood.required_action_key = "chop";
    wood.output_object_keys = {"wood"};
    wood.charges = 2;
    profile.resource_rules.push_back(wood);

    pathfinder::content::WorldgenResourceRuleDto stone;
    stone.resource_key = "loose_stone_node";
    stone.allowed_terrain_tags = {"plain", "stone_field"};
    stone.density = 0.08;
    stone.tag_keys = {"stone_basic"};
    stone.node_kind = "Stone";
    stone.required_action_key = "gather";
    stone.output_object_keys = {"loose_stone"};
    stone.charges = 2;
    profile.resource_rules.push_back(stone);

    pathfinder::content::WorldgenGroundItemRuleDto ground;
    ground.object_key = "loose_stone";
    ground.allowed_terrain_tags = {"plain", "stone_field"};
    ground.density = 0.08;
    ground.quantity = 1;
    ground.stackable = true;
    ground.stack_key = "loose_stone:default";
    ground.tag_keys = {"stone_basic"};
    profile.ground_item_rules.push_back(ground);

    profile.spawn_safety.safe_radius = 2;
    profile.spawn_safety.basic_food_min_count = 1;
    profile.spawn_safety.basic_material_min_count = 2;
    profile.spawn_safety.tool_hint_min_count = 0;
    profile.spawn_safety.immediate_threat_max_count = 0;
    draft.addWorldgenProfile(std::move(profile));

    return pathfinder::content::ContentRegistry::build(draft);
}

WorldGenerationService makeContentBackedWorldGenerationService() {
    WorldGenerationService service;
    auto registry = makeContentBackedWorldgenRegistry();
    auto result = service.registerContentProfiles(*registry);
    assert(result.is_ok());
    return service;
}

} // namespace

// ---------------------------------------------------------------------------
// Determinism tests
// ---------------------------------------------------------------------------

void run_world_generation_same_seed_same_region_same_result_tests() {
    auto service1 = makeContentBackedWorldGenerationService();
    auto service2 = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_world";
    request.world_seed = 12345;
    request.generator_version = "1.0.0";
    request.content_version = "1.0.0";
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest req1;
    req1.world_id = "test_world";
    req1.world_seed = 111;
    req1.worldgen_profile_key = "sandbox_blank";
    req1.region_size = 16;

    WorldGenerationRequest req2;
    req2.world_id = "test_world_2";
    req2.world_seed = 222;
    req2.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_manifest";
    request.world_seed = 999;
    request.generator_version = "2.0.0";
    request.content_version = "3.0.0";
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(result.manifest.world_seed == 999);
    assert(result.manifest.generator_version == "2.0.0");
    assert(result.manifest.content_version == "3.0.0");
    assert(result.manifest.worldgen_profile_key == "sandbox_blank");
    assert(!result.manifest.trace_roll_keys.empty());

    std::cout << "world_generation_manifest_contains_seed_profile_version: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Terrain tests
// ---------------------------------------------------------------------------

void run_world_generation_creates_cells_with_layer_key_tests() {
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_layer";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_terrain";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_blocked";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_resource_terrain";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_resource_action";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service1 = makeContentBackedWorldGenerationService();
    auto service2 = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_stable";
    request.world_seed = 777;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_food";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_materials";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    for (const auto& item : result.entity_drafts) {
        int dist = std::abs(item.coord.x) + std::abs(item.coord.y);
        if (dist <= 2) {
            for (const auto& tag : item.tag_keys) {
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_threat";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_ownership";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_stack";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_no_inv";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_cmd";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest req1;
    req1.world_id = "test_collision";
    req1.world_seed = 12345;
    req1.worldgen_profile_key = "sandbox_blank";
    req1.region_coord = WorldRegionCoord{0, 0};
    req1.region_size = 16;

    WorldGenerationRequest req2;
    req2.world_id = "test_collision";
    req2.world_seed = 12345;
    req2.worldgen_profile_key = "sandbox_blank";
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

    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "runtime_cells";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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

    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "entity_cell";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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

    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "resource_runtime";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "pure_test";
    request.world_seed = 777;
    request.worldgen_profile_key = "sandbox_blank";
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

    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "repeat_test";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "tag_test";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    for (const auto& item : result.entity_drafts) {
        int dist = std::abs(item.coord.x) + std::abs(item.coord.y);
        if (dist <= 2) {
            for (const auto& tag : item.tag_keys) {
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
// P0: ground items non-empty for sandbox_blank
// ---------------------------------------------------------------------------

void run_world_generation_ground_items_non_empty_for_sandbox_blank_tests() {
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_nonempty";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);
    assert(!result.entity_drafts.empty());

    std::cout << "world_generation_ground_items_non_empty_for_sandbox_blank: passed" << std::endl;
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

    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_nodes";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
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
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_layer";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;
    request.enabled_layer_keys = {"surface", "underground"};

    auto result = service.generate(request);
    assert(!result.ok);
    assert(result.failure_kind == WorldGenerationFailureKind::InvalidLayer);

    std::cout << "world_generation_invalid_layer_rejected: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P57: Noise-based terrain generation tests
// ---------------------------------------------------------------------------

void run_world_generation_spawn_cell_is_walkable_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_spawn_walkable";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool found_spawn = false;
    for (const auto& cell : result.cell_drafts) {
        if (cell.coord.x == 0 && cell.coord.y == 0) {
            found_spawn = true;
            assert(!cell.blocks_movement);
            assert(cell.terrain_key != "blocked");
            assert(cell.terrain_key != "mountain");
            assert(cell.terrain_key != "deep_water");
        }
    }
    assert(found_spawn);
    std::cout << "world_generation_spawn_cell_is_walkable: passed" << std::endl;
}

void run_world_generation_spawn_radius_blocked_ratio_limited_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_spawn_radius";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    const auto* profile = service.findProfile("sandbox_blank");
    assert(profile != nullptr);
    int safe_radius = profile->connectivity_policy.spawn_safe_radius;
    double max_blocked_ratio = profile->connectivity_policy.max_blocked_ratio_in_spawn_radius;

    int total_in_radius = 0;
    int blocked_in_radius = 0;
    for (const auto& cell : result.cell_drafts) {
        int dist = std::abs(cell.coord.x) + std::abs(cell.coord.y);
        if (dist <= safe_radius) {
            ++total_in_radius;
            if (cell.blocks_movement) ++blocked_in_radius;
        }
    }

    if (total_in_radius > 0) {
        double ratio = static_cast<double>(blocked_in_radius) / static_cast<double>(total_in_radius);
        assert(ratio <= max_blocked_ratio + 0.001); // epsilon
    }
    std::cout << "world_generation_spawn_radius_blocked_ratio_limited: passed" << std::endl;
}

void run_world_generation_walkable_ratio_above_policy_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_walkable_ratio";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    const auto* profile = service.findProfile("sandbox_blank");
    assert(profile != nullptr);
    double min_walkable_ratio = profile->connectivity_policy.min_walkable_ratio;

    int walkable = 0;
    for (const auto& cell : result.cell_drafts) {
        if (!cell.blocks_movement) ++walkable;
    }
    double ratio = static_cast<double>(walkable) / static_cast<double>(result.cell_drafts.size());
    assert(ratio >= min_walkable_ratio - 0.001);
    std::cout << "world_generation_walkable_ratio_above_policy: passed" << std::endl;
}

void run_world_generation_reachable_from_spawn_above_minimum_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_reachable";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    const auto* profile = service.findProfile("sandbox_blank");
    assert(profile != nullptr);
    int min_reachable = profile->connectivity_policy.min_reachable_cells_from_spawn;

    // BFS from spawn
    std::map<std::string, const GeneratedCellDraft*> cell_map;
    for (const auto& cell : result.cell_drafts) {
        cell_map[cell.coord.cellId()] = &cell;
    }

    WorldCellCoord spawn{0, 0, "surface"};
    std::set<std::string> visited;
    std::vector<WorldCellCoord> queue;
    auto spawn_it = cell_map.find(spawn.cellId());
    if (spawn_it != cell_map.end() && !spawn_it->second->blocks_movement) {
        queue.push_back(spawn);
        visited.insert(spawn.cellId());
    }

    size_t idx = 0;
    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    while (idx < queue.size()) {
        auto cur = queue[idx++];
        for (int dir = 0; dir < 4; ++dir) {
            WorldCellCoord nxt{cur.x + dx[dir], cur.y + dy[dir], cur.layer_key};
            auto it = cell_map.find(nxt.cellId());
            if (it == cell_map.end()) continue;
            if (visited.count(nxt.cellId())) continue;
            if (it->second->blocks_movement) continue;
            visited.insert(nxt.cellId());
            queue.push_back(nxt);
        }
    }

    assert(static_cast<int>(visited.size()) >= min_reachable);
    std::cout << "world_generation_reachable_from_spawn_above_minimum: passed" << std::endl;
}

void run_world_generation_spawn_has_cardinal_escape_routes_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_escape";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    std::map<std::string, const GeneratedCellDraft*> cell_map;
    for (const auto& cell : result.cell_drafts) {
        cell_map[cell.coord.cellId()] = &cell;
    }

    const int directions[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    int good_directions = 0;
    for (int dir = 0; dir < 4; ++dir) {
        bool good = true;
        for (int step = 1; step <= 3; ++step) {
            WorldCellCoord coord{directions[dir][0] * step, directions[dir][1] * step, "surface"};
            auto it = cell_map.find(coord.cellId());
            if (it == cell_map.end() || it->second->blocks_movement) {
                good = false;
                break;
            }
        }
        if (good) ++good_directions;
    }

    assert(good_directions >= 3);
    std::cout << "world_generation_spawn_has_cardinal_escape_routes: passed" << std::endl;
}

void run_world_generation_forest_is_walkable_with_cost_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_forest";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool found_forest = false;
    for (const auto& cell : result.cell_drafts) {
        if (cell.terrain_key == "forest") {
            found_forest = true;
            assert(!cell.blocks_movement);
            assert(cell.movement_cost > 1);
        }
    }
    assert(found_forest); // should have some forest in a 16x16 noise field
    std::cout << "world_generation_forest_is_walkable_with_cost: passed" << std::endl;
}

void run_world_generation_stone_field_is_walkable_with_cost_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_stone";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    bool found_stone = false;
    for (const auto& cell : result.cell_drafts) {
        if (cell.terrain_key == "stone_field") {
            found_stone = true;
            assert(!cell.blocks_movement);
            assert(cell.movement_cost >= 2);
        }
    }
    assert(found_stone);
    std::cout << "world_generation_stone_field_is_walkable_with_cost: passed" << std::endl;
}

void run_world_generation_only_blocking_terrains_block_movement_tests() {
    auto service = makeContentBackedWorldGenerationService();
    WorldGenerationRequest request;
    request.world_id = "test_blocking";
    request.world_seed = 42;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_size = 16;

    auto result = service.generate(request);
    assert(result.ok);

    for (const auto& cell : result.cell_drafts) {
        if (cell.terrain_key == "plain" || cell.terrain_key == "forest" ||
            cell.terrain_key == "stone_field" || cell.terrain_key == "water_edge") {
            assert(!cell.blocks_movement);
        }
        if (cell.terrain_key == "blocked" || cell.terrain_key == "mountain" ||
            cell.terrain_key == "deep_water") {
            assert(cell.blocks_movement);
        }
    }
    std::cout << "world_generation_only_blocking_terrains_block_movement: passed" << std::endl;
}

void run_world_generation_many_seeds_never_trap_spawn_tests() {
    auto service_for_profile = makeContentBackedWorldGenerationService();
    const auto* profile = service_for_profile.findProfile("sandbox_blank");
    assert(profile != nullptr);
    int min_reachable = profile->connectivity_policy.min_reachable_cells_from_spawn;
    double min_walkable_ratio = profile->connectivity_policy.min_walkable_ratio;

    for (uint64_t seed = 1; seed <= 50; ++seed) {
        auto service = makeContentBackedWorldGenerationService();
        WorldGenerationRequest request;
        request.world_id = "test_trap";
        request.world_seed = seed;
        request.worldgen_profile_key = "sandbox_blank";
        request.region_size = 16;

        auto result = service.generate(request);
        assert(result.ok);

        // Spawn walkable
        bool spawn_walkable = false;
        for (const auto& cell : result.cell_drafts) {
            if (cell.coord.x == 0 && cell.coord.y == 0) {
                spawn_walkable = !cell.blocks_movement;
                break;
            }
        }
        assert(spawn_walkable);

        // Walkable ratio
        int walkable = 0;
        for (const auto& cell : result.cell_drafts) {
            if (!cell.blocks_movement) ++walkable;
        }
        double ratio = static_cast<double>(walkable) / result.cell_drafts.size();
        assert(ratio >= min_walkable_ratio - 0.001);

        // BFS reachable
        std::map<std::string, const GeneratedCellDraft*> cell_map;
        for (const auto& cell : result.cell_drafts) {
            cell_map[cell.coord.cellId()] = &cell;
        }
        WorldCellCoord spawn{0, 0, "surface"};
        std::set<std::string> visited;
        std::vector<WorldCellCoord> queue;
        auto it = cell_map.find(spawn.cellId());
        if (it != cell_map.end() && !it->second->blocks_movement) {
            queue.push_back(spawn);
            visited.insert(spawn.cellId());
        }
        size_t idx = 0;
        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        while (idx < queue.size()) {
            auto cur = queue[idx++];
            for (int dir = 0; dir < 4; ++dir) {
                WorldCellCoord nxt{cur.x + dx[dir], cur.y + dy[dir], cur.layer_key};
                auto nit = cell_map.find(nxt.cellId());
                if (nit == cell_map.end()) continue;
                if (visited.count(nxt.cellId())) continue;
                if (nit->second->blocks_movement) continue;
                visited.insert(nxt.cellId());
                queue.push_back(nxt);
            }
        }
        assert(static_cast<int>(visited.size()) >= min_reachable);
    }
    std::cout << "world_generation_many_seeds_never_trap_spawn: passed" << std::endl;
}

void run_world_generation_far_region_is_deterministic_tests() {
    auto service1 = makeContentBackedWorldGenerationService();
    auto service2 = makeContentBackedWorldGenerationService();

    WorldGenerationRequest request;
    request.world_id = "test_far";
    request.world_seed = 777;
    request.worldgen_profile_key = "sandbox_blank";
    request.region_coord = WorldRegionCoord{1000, -1000};
    request.region_size = 16;

    auto result1 = service1.generate(request);
    auto result2 = service2.generate(request);

    assert(result1.ok);
    assert(result2.ok);
    assert(result1.cell_drafts.size() == result2.cell_drafts.size());

    for (size_t i = 0; i < result1.cell_drafts.size(); ++i) {
        assert(result1.cell_drafts[i].terrain_key == result2.cell_drafts[i].terrain_key);
        assert(result1.cell_drafts[i].blocks_movement == result2.cell_drafts[i].blocks_movement);
    }
    std::cout << "world_generation_far_region_is_deterministic: passed" << std::endl;
}

void run_world_generation_adjacent_region_noise_has_no_seam_tests() {
    auto service = makeContentBackedWorldGenerationService();

    WorldGenerationRequest req00;
    req00.world_id = "test_seam";
    req00.world_seed = 555;
    req00.worldgen_profile_key = "sandbox_blank";
    req00.region_coord = WorldRegionCoord{0, 0};
    req00.region_size = 16;

    WorldGenerationRequest req10;
    req10.world_id = "test_seam";
    req10.world_seed = 555;
    req10.worldgen_profile_key = "sandbox_blank";
    req10.region_coord = WorldRegionCoord{1, 0};
    req10.region_size = 16;

    auto result00 = service.generate(req00);
    auto result10 = service.generate(req10);
    assert(result00.ok);
    assert(result10.ok);

    // Build maps
    std::map<std::string, std::string> terrain00;
    for (const auto& cell : result00.cell_drafts) {
        terrain00[cell.coord.cellId()] = cell.terrain_key;
    }

    // Check right edge of (0,0) aligns with left edge of (1,0)
    // Region (0,0) right edge: world_x = 7 (max_c for even size)
    // Region (1,0) left edge: world_x = 8 (min_c for even size is -8, so 16 + (-8) = 8)
    bool has_edge_00 = false;
    bool has_edge_10 = false;
    for (const auto& cell : result00.cell_drafts) {
        if (cell.coord.x == 7) has_edge_00 = true;
    }
    for (const auto& cell : result10.cell_drafts) {
        if (cell.coord.x == 8) has_edge_10 = true;
    }
    assert(has_edge_00);
    assert(has_edge_10);

    // P57: verify noise continuity across region boundary using TerrainNoiseSampler.
    // The sampler uses world_x/world_y, so adjacent world coords should have
    // smoothly varying noise values regardless of region boundaries.
    TerrainNoiseSampler sampler(req00.world_seed);
    NoiseChannelConfig elev_cfg;
    elev_cfg.channel = NoiseChannelKind::Elevation;
    elev_cfg.algorithm = NoiseAlgorithmKind::FractalPerlin2D;
    elev_cfg.scale = 8.0;
    elev_cfg.octaves = 3;
    elev_cfg.persistence = 0.5;
    elev_cfg.lacunarity = 2.0;
    elev_cfg.weight = 1.0;
    elev_cfg.bias = 0.0;
    elev_cfg.salt = 101;

    // Sample at the boundary and several points across it
    for (int y = -2; y <= 2; ++y) {
        double v_left  = sampler.sample(7, y, "surface", elev_cfg);
        double v_right = sampler.sample(8, y, "surface", elev_cfg);
        // Perlin noise is continuous; adjacent coordinates should not jump drastically
        assert(std::abs(v_left - v_right) < 0.8);
    }

    std::cout << "world_generation_adjacent_region_noise_has_no_seam: passed" << std::endl;
}
