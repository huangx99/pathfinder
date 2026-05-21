#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_reaction/craft_command_handler.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include "pathfinder/content/content_registry.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_reaction;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::world_harvest;
using namespace pathfinder::world_command;
using namespace pathfinder::content;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<ContentRegistry> makeTestContentRegistry() {
    ContentDraftRegistry draft;

    ObjectDefinitionContent wood_obj;
    wood_obj.key = ObjectDefinitionKey("wood");
    wood_obj.display_key = "entity.wood";
    draft.addObject(std::move(wood_obj));

    ObjectDefinitionContent torch_obj;
    torch_obj.key = ObjectDefinitionKey("torch");
    torch_obj.display_key = "entity.torch";
    draft.addObject(std::move(torch_obj));

    ObjectDefinitionContent fire_obj;
    fire_obj.key = ObjectDefinitionKey("camp_fire");
    fire_obj.display_key = "entity.camp_fire";
    draft.addObject(std::move(fire_obj));

    ReactionDefinitionContent reaction;
    reaction.key = ReactionDefinitionKey("wood_plus_fire_make_torch");
    reaction.inputs = {
        ReactionInputDto{"wood", "processed", 1, 0},
        ReactionInputDto{"camp_fire", "", 0, 1}
    };
    reaction.action_key = "use";
    reaction.effect_key = "make_torch";
    reaction.outputs = {
        ReactionOutputDto{"torch", 1}
    };
    reaction.consume = {
        ReactionConsumeDto{"wood", "processed", -1}
    };
    reaction.summary_key = "reaction.make_torch.summary";
    reaction.knowledge_templates = {"knowledge_make_torch"};
    draft.addReaction(std::move(reaction));

    return ContentRegistry::build(draft);
}

static void addProcessedWoodToInventory(WorldInventoryRuntime& inv, const std::string& actor_key, int quantity) {
    InventoryTransferRequest req;
    req.transfer_id = "setup_wood";
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = actor_key;
    req.entity_key = "wood";
    req.quantity = quantity;
    req.parameters["stack_key"] = "wood:processed";
    req.parameters["state_keys"] = "processed";
    auto res = inv.transfer(req);
    assert(res.is_ok() && res.value().ok);
}

static void addCampFireToMap(WorldGridRuntime& world, const WorldCellCoord& coord) {
    auto spawn = world.spawnEntityOnMap(
        "camp_fire_001", "camp_fire", "entity.camp_fire",
        coord, 1, "camp_fire:default", true,
        std::vector<std::string>{}, {}, {});
    assert(spawn.is_ok());
}

// ---------------------------------------------------------------------------
// Craft reaction key from target recipe
// ---------------------------------------------------------------------------

void run_world_command_craft_reaction_key_from_target_recipe_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    auto registry = makeTestContentRegistry();
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto handler = createCraftCommandHandler(service, world_runtime, inventory_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldCommandDto cmd;
    cmd.command_id = "cmd1";
    cmd.command_kind = WorldCommandKind::Craft;
    cmd.actor_key = "player";
    cmd.target.recipe_key = "wood_plus_fire_make_torch";

    WorldCommandContext context(cmd);
    context.setCurrentTick(77);

    auto result = handler->execute(context, cmd);
    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);
    assert(!result.value().events.empty());
    assert(result.value().events[0].tick == 77);

    std::cout << "world_command_craft_reaction_key_from_target_recipe: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Craft events use context tick
// ---------------------------------------------------------------------------

void run_world_command_craft_events_use_context_tick_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    auto registry = makeTestContentRegistry();
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto handler = createCraftCommandHandler(service, world_runtime, inventory_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldCommandDto cmd;
    cmd.command_id = "cmd1";
    cmd.command_kind = WorldCommandKind::Craft;
    cmd.actor_key = "player";
    cmd.target.recipe_key = "wood_plus_fire_make_torch";

    WorldCommandContext context(cmd);
    context.setCurrentTick(99);

    auto result = handler->execute(context, cmd);
    assert(result.is_ok());
    assert(result.value().result_kind == WorldCommandResultKind::Succeeded);

    // All success events must use context tick
    for (const auto& event : result.value().events) {
        assert(event.tick == 99);
    }

    // Experience tick must match
    for (const auto& exp : result.value().experiences) {
        assert(exp.tick == 99);
    }

    std::cout << "world_command_craft_events_use_context_tick: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Harvest pickup then craft torch end-to-end
// ---------------------------------------------------------------------------

void run_world_generation_harvest_pickup_then_craft_torch_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    ResourceHarvestService harvest_service(world_runtime, world_runtime, inventory_runtime);

    auto registry = makeTestContentRegistry();
    WorldReactionService reaction_service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto craft_handler = createCraftCommandHandler(reaction_service, world_runtime, inventory_runtime);

    // Create a young_tree resource node and chop it
    WorldResourceNodeRuntime node;
    node.node_id = "test_tree_001";
    node.resource_key = "young_tree";
    node.coord = WorldCellCoord{0, 1, "surface"};
    node.node_kind_str = "plant";
    node.node_state_str = "Available";
    node.remaining_charges = 3;
    node.max_charges = 3;
    node.required_action_key = "chop";
    node.required_tool_key = "";
    node.output_entity_keys = {"wood"};
    world_runtime.upsertGeneratedResourceNode(node);

    // Chop to create wood on map
    WorldCommandDto chop_cmd;
    chop_cmd.command_id = "chop1";
    chop_cmd.command_kind = WorldCommandKind::Chop;
    chop_cmd.actor_key = "player";
    chop_cmd.target.target_entity_id = "test_tree_001";

    WorldCommandContext chop_context(chop_cmd);
    chop_context.setCurrentTick(1);

    auto chop_handler = createChopCommandHandler(harvest_service);
    auto chop_result = chop_handler->execute(chop_context, chop_cmd);
    assert(chop_result.is_ok());
    assert(chop_result.value().result_kind == WorldCommandResultKind::Succeeded);

    // Find the spawned wood entity
    std::string wood_entity_id;
    for (const auto& eid : chop_result.value().changed_entity_ids) {
        auto ent_res = world_runtime.findEntity(eid);
        if (ent_res.is_ok() && ent_res.value()->entity_key == "wood") {
            wood_entity_id = eid;
            break;
        }
    }
    assert(!wood_entity_id.empty());

    // Pickup wood
    WorldCommandDto pickup_cmd;
    pickup_cmd.command_id = "pickup1";
    pickup_cmd.command_kind = WorldCommandKind::Pickup;
    pickup_cmd.actor_key = "player";
    pickup_cmd.target.target_entity_id = wood_entity_id;

    WorldCommandContext pickup_context(pickup_cmd);
    pickup_context.setCurrentTick(2);

    auto pickup_handler = createPickupCommandHandler(inventory_runtime, world_runtime);
    auto pickup_result = pickup_handler->execute(pickup_context, pickup_cmd);
    assert(pickup_result.is_ok());
    assert(pickup_result.value().result_kind == WorldCommandResultKind::Succeeded);

    // Replace wood with processed wood (test helper)
    {
        auto inv_id = inventory_runtime.ensureActorInventory("player");
        auto inv_res = inventory_runtime.findInventory(inv_id.value());
        assert(inv_res.is_ok());
        for (auto& entry : inv_res.value()->entries) {
            if (entry.entity_key == "wood") {
                entry.state_keys = {"processed"};
                entry.stack_key = "wood:processed";
                break;
            }
        }
    }

    // Spawn camp_fire at player location
    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;
    addCampFireToMap(world_runtime, coord);

    // Craft torch
    WorldCommandDto craft_cmd;
    craft_cmd.command_id = "craft1";
    craft_cmd.command_kind = WorldCommandKind::Craft;
    craft_cmd.actor_key = "player";
    craft_cmd.target.recipe_key = "wood_plus_fire_make_torch";

    WorldCommandContext craft_context(craft_cmd);
    craft_context.setCurrentTick(3);

    auto craft_result = craft_handler->execute(craft_context, craft_cmd);
    assert(craft_result.is_ok());
    assert(craft_result.value().result_kind == WorldCommandResultKind::Succeeded);

    // Verify wood consumed, torch in inventory
    auto inv_id = inventory_runtime.ensureActorInventory("player");
    auto inv_res = inventory_runtime.findInventory(inv_id.value());
    assert(inv_res.is_ok());

    bool has_wood = false;
    bool has_torch = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "wood") has_wood = true;
        if (entry.entity_key == "torch") has_torch = true;
    }
    assert(!has_wood);
    assert(has_torch);

    std::cout << "world_generation_harvest_pickup_then_craft_torch: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// DropOnMap output projection patch
// ---------------------------------------------------------------------------

void run_world_command_craft_drop_on_map_projection_patch_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    auto registry = makeTestContentRegistry();
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto handler = createCraftCommandHandler(service, world_runtime, inventory_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    // Spawn processed wood in inventory and camp_fire on map
    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    // Build craft command with DropOnMap policy
    WorldCommandDto cmd;
    cmd.command_id = "craft_cmd_1";
    cmd.command_kind = WorldCommandKind::Craft;
    cmd.actor_key = "player";
    cmd.target.recipe_key = "wood_plus_fire_make_torch";
    cmd.parameters["output_location_policy"] = "drop_on_map";

    WorldCommandContext context(cmd);
    context.setCurrentTick(123);

    auto exec_res = handler->execute(context, cmd);
    assert(exec_res.is_ok());

    auto result = exec_res.value();
    assert(result.result_kind == WorldCommandResultKind::Succeeded);
    assert(result.projection_patch_override.has_value());

    auto& patch = result.projection_patch_override.value();
    bool found_torch = false;
    for (const auto& entity_patch : patch.changed_entities) {
        if (entity_patch.fields.at("entity_key") == "torch") {
            found_torch = true;
            break;
        }
    }
    assert(found_torch);
    assert(patch.new_projection_version > patch.base_projection_version);

    std::cout << "world_command_craft_drop_on_map_projection_patch: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Projection patch version advances
// ---------------------------------------------------------------------------

void run_world_command_craft_projection_version_advances_tests() {
    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 9;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    WorldGridRuntime world_runtime;
    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);

    WorldInventoryRuntime inventory_runtime(world_runtime);
    inventory_runtime.initialize();

    auto registry = makeTestContentRegistry();
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);
    auto handler = createCraftCommandHandler(service, world_runtime, inventory_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    uint64_t version_before = world_runtime.stateVersion();

    WorldCommandDto cmd;
    cmd.command_id = "craft_cmd_2";
    cmd.command_kind = WorldCommandKind::Craft;
    cmd.actor_key = "player";
    cmd.target.recipe_key = "wood_plus_fire_make_torch";

    WorldCommandContext context(cmd);
    context.setCurrentTick(124);

    auto exec_res = handler->execute(context, cmd);
    assert(exec_res.is_ok());

    auto result = exec_res.value();
    assert(result.result_kind == WorldCommandResultKind::Succeeded);
    assert(result.projection_patch_override.has_value());

    auto& patch = result.projection_patch_override.value();
    assert(patch.base_projection_version == version_before);
    assert(patch.new_projection_version == version_before + 1);

    std::cout << "world_command_craft_projection_version_advances: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests:" << std::endl;
        std::cout << "  craft_reaction_key_from_target_recipe" << std::endl;
        std::cout << "  craft_events_use_context_tick" << std::endl;
        std::cout << "  harvest_pickup_then_craft_torch" << std::endl;
        std::cout << "  craft_drop_on_map_projection_patch" << std::endl;
        std::cout << "  craft_projection_version_advances" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "craft_reaction_key_from_target_recipe") {
        run_world_command_craft_reaction_key_from_target_recipe_tests();
    } else if (test_name == "craft_events_use_context_tick") {
        run_world_command_craft_events_use_context_tick_tests();
    } else if (test_name == "harvest_pickup_then_craft_torch") {
        run_world_generation_harvest_pickup_then_craft_torch_tests();
    } else if (test_name == "craft_drop_on_map_projection_patch") {
        run_world_command_craft_drop_on_map_projection_patch_tests();
    } else if (test_name == "craft_projection_version_advances") {
        run_world_command_craft_projection_version_advances_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
