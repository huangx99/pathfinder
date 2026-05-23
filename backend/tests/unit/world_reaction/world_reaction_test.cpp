#include "pathfinder/world_reaction/world_reaction_types.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/content/content_registry.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_reaction;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;
using namespace pathfinder::content;
using namespace pathfinder::world_command;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::shared_ptr<ContentRegistry> makeTestContentRegistry(
    const ReactionDefinitionContent& reaction) {
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

    ObjectDefinitionContent loose_stone_obj;
    loose_stone_obj.key = ObjectDefinitionKey("loose_stone");
    loose_stone_obj.display_key = "entity.loose_stone";
    draft.addObject(std::move(loose_stone_obj));

    ObjectDefinitionContent axe_obj;
    axe_obj.key = ObjectDefinitionKey("axe");
    axe_obj.display_key = "entity.axe";
    draft.addObject(std::move(axe_obj));

    draft.addReaction(reaction);
    return ContentRegistry::build(draft);
}

static ReactionDefinitionContent makeTorchReaction(int wood_min = 1, int wood_consume = -1) {
    ReactionDefinitionContent reaction;
    reaction.key = ReactionDefinitionKey("wood_plus_fire_make_torch");
    reaction.inputs = {
        ReactionInputDto{"wood", "processed", wood_min, 0},
        ReactionInputDto{"camp_fire", "", 0, 1}
    };
    reaction.action_key = "use";
    reaction.effect_key = "make_torch";
    reaction.outputs = {
        ReactionOutputDto{"torch", 1}
    };
    reaction.consume = {
        ReactionConsumeDto{"wood", "processed", wood_consume}
    };
    reaction.summary_key = "reaction.make_torch.summary";
    reaction.knowledge_templates = {"knowledge_make_torch"};
    return reaction;
}

static ReactionDefinitionContent makeAxeReaction() {
    ReactionDefinitionContent reaction;
    reaction.key = ReactionDefinitionKey("loose_stone_plus_wood_make_axe");
    reaction.inputs = {
        ReactionInputDto{"loose_stone", "", 1, 0},
        ReactionInputDto{"wood", "", 1, 0}
    };
    reaction.action_key = "craft";
    reaction.effect_key = "make_axe";
    reaction.outputs = {
        ReactionOutputDto{"axe", 1}
    };
    reaction.consume = {
        ReactionConsumeDto{"loose_stone", "", -1},
        ReactionConsumeDto{"wood", "", -1}
    };
    reaction.summary_key = "reaction.make_axe.summary";
    reaction.knowledge_templates = {"knowledge_make_axe"};
    return reaction;
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

static void addItemToInventory(WorldInventoryRuntime& inv, const std::string& actor_key, const std::string& entity_key, int quantity) {
    InventoryTransferRequest req;
    req.transfer_id = "setup_" + entity_key;
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = actor_key;
    req.entity_key = entity_key;
    req.quantity = quantity;
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
// Apply craft torch to inventory
// ---------------------------------------------------------------------------

void run_world_reaction_apply_craft_torch_to_inventory_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    // Setup: processed wood in inventory + camp_fire on map
    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;
    req.output_location_policy = WorldReactionOutputLocationPolicy::ActorInventory;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    // Wood consumed
    auto inv_id = inventory_runtime.ensureActorInventory("player");
    auto inv_res = inventory_runtime.findInventory(inv_id.value());
    assert(inv_res.is_ok());
    bool has_wood = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "wood") {
            has_wood = true;
            break;
        }
    }
    assert(!has_wood);

    // Camp fire still on map
    auto fire_res = world_runtime.findEntity("camp_fire_001");
    assert(fire_res.is_ok());
    assert(fire_res.value()->location_kind == WorldEntityLocationKind::OnMap);

    // Torch in inventory
    bool has_torch = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "torch") {
            has_torch = true;
            assert(entry.quantity == 1);
            break;
        }
    }
    assert(has_torch);

    // Events
    assert(!result.value().events.empty());

    // State deltas
    assert(!result.value().state_deltas.empty());

    // Experiences
    assert(!result.value().experiences.empty());
    assert(result.value().experiences[0].subject_entity_key == "torch");
    assert(result.value().experiences[0].effect_key == "make_torch");

    std::cout << "world_reaction_apply_craft_torch_to_inventory: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Apply craft axe to inventory
// ---------------------------------------------------------------------------

void run_world_reaction_apply_craft_axe_to_inventory_tests() {
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

    auto registry = makeTestContentRegistry(makeAxeReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    addItemToInventory(inventory_runtime, "player", "loose_stone", 1);
    addItemToInventory(inventory_runtime, "player", "wood", 1);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd_axe";
    req.actor_key = "player";
    req.reaction_key = "loose_stone_plus_wood_make_axe";
    req.action_kind = WorldReactionActionKind::Craft;
    req.output_location_policy = WorldReactionOutputLocationPolicy::ActorInventory;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    InventoryOwnerRef owner;
    owner.owner_kind = InventoryOwnerKind::Actor;
    owner.owner_key = "player";
    assert(inventory_runtime.queryItems(owner, "loose_stone").value().empty());
    assert(inventory_runtime.queryItems(owner, "wood").value().empty());
    auto axes = inventory_runtime.queryItems(owner, "axe").value();
    assert(axes.size() == 1);
    assert(axes[0].quantity == 1);

    assert(!result.value().experiences.empty());
    assert(result.value().experiences[0].subject_entity_key == "axe");
    assert(result.value().experiences[0].effect_key == "make_axe");

    std::cout << "world_reaction_apply_craft_axe_to_inventory: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate input missing
// ---------------------------------------------------------------------------

void run_world_reaction_validate_input_missing_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(!result.value().ok);
    assert(result.value().failure_kind == WorldReactionFailureKind::InputMissing);

    std::cout << "world_reaction_validate_input_missing: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate state mismatch
// ---------------------------------------------------------------------------

void run_world_reaction_validate_state_mismatch_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    // Add wood WITHOUT processed state
    InventoryTransferRequest spawn_req;
    spawn_req.transfer_id = "setup_wood";
    spawn_req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    spawn_req.actor_key = "player";
    spawn_req.entity_key = "wood";
    spawn_req.quantity = 1;
    spawn_req.parameters["stack_key"] = "wood:default";
    auto sres = inventory_runtime.transfer(spawn_req);
    assert(sres.is_ok() && sres.value().ok);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    addCampFireToMap(world_runtime, actor_res.value()->coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(!result.value().ok);
    // Should be InputMissing because wood without processed state is not a match
    assert(result.value().failure_kind == WorldReactionFailureKind::InputStateMismatch);

    // Wood not consumed
    auto inv_id = inventory_runtime.ensureActorInventory("player");
    auto inv_res = inventory_runtime.findInventory(inv_id.value());
    assert(inv_res.is_ok());
    bool has_wood = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "wood") {
            has_wood = true;
            assert(entry.quantity == 1);
            break;
        }
    }
    assert(has_wood);

    std::cout << "world_reaction_validate_state_mismatch: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate quantity insufficient
// ---------------------------------------------------------------------------

void run_world_reaction_validate_quantity_insufficient_tests() {
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

    // Reaction requires wood min=2
    auto registry = makeTestContentRegistry(makeTorchReaction(2, -2));
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    addProcessedWoodToInventory(inventory_runtime, "player", 1);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    addCampFireToMap(world_runtime, actor_res.value()->coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(!result.value().ok);
    assert(result.value().failure_kind == WorldReactionFailureKind::QuantityInsufficient);

    std::cout << "world_reaction_validate_quantity_insufficient: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate catalyst not consumed
// ---------------------------------------------------------------------------

void run_world_reaction_validate_catalyst_not_consumed_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    addProcessedWoodToInventory(inventory_runtime, "player", 1);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    addCampFireToMap(world_runtime, actor_res.value()->coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto draft_res = service.validate(req);
    assert(draft_res.is_ok());

    // consume_drafts should only contain wood
    assert(draft_res.value().consume_drafts.size() == 1);
    assert(draft_res.value().consume_drafts[0].object_key == "wood");

    // camp_fire should be in input_proofs but not in consume_drafts
    bool has_camp_fire_proof = false;
    for (const auto& proof : draft_res.value().input_proofs) {
        if (proof.object_key == "camp_fire") {
            has_camp_fire_proof = true;
            break;
        }
    }
    assert(has_camp_fire_proof);

    std::cout << "world_reaction_validate_catalyst_not_consumed: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Failure does not pollute state
// ---------------------------------------------------------------------------

void run_world_reaction_failure_does_not_pollute_state_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    // No camp_fire

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(!result.value().ok);

    // Wood untouched
    auto inv_id = inventory_runtime.ensureActorInventory("player");
    auto inv_res = inventory_runtime.findInventory(inv_id.value());
    assert(inv_res.is_ok());
    bool has_wood = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "wood") {
            has_wood = true;
            assert(entry.quantity == 1);
            break;
        }
    }
    assert(has_wood);

    // No torch spawned
    bool has_torch = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "torch") {
            has_torch = true;
            break;
        }
    }
    assert(!has_torch);

    std::cout << "world_reaction_failure_does_not_pollute_state: passed" << std::endl;
}


// ---------------------------------------------------------------------------
// Craft output entity exists in world runtime
// ---------------------------------------------------------------------------

void run_world_reaction_craft_output_entity_exists_in_world_runtime_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    // Find torch entity id from state deltas
    std::string torch_entity_id;
    for (const auto& delta : result.value().state_deltas) {
        if (delta.delta_kind == "item_created" && delta.fields.at("object_key") == "torch") {
            torch_entity_id = delta.target_ref;
            break;
        }
    }
    assert(!torch_entity_id.empty());

    auto ent_res = world_runtime.findEntity(torch_entity_id);
    assert(ent_res.is_ok());
    const auto* entity = ent_res.value();
    assert(entity->location_kind == WorldEntityLocationKind::InInventory);
    assert(entity->entity_key == "torch");
    assert(entity->quantity == 1);

    bool has_crafted = false;
    for (const auto& sk : entity->state_keys) {
        if (sk == "crafted") has_crafted = true;
    }
    assert(has_crafted);

    std::cout << "world_reaction_craft_output_entity_exists_in_world_runtime: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Output id is command deterministic
// ---------------------------------------------------------------------------

void run_world_reaction_output_id_is_command_deterministic_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "craft1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto draft_res = service.validate(req);
    assert(draft_res.is_ok());
    assert(!draft_res.value().output_drafts.empty());
    assert(draft_res.value().output_drafts[0].entity_id == "craft1_torch_0");

    std::cout << "world_reaction_output_id_is_command_deterministic: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Consumes same cell map input
// ---------------------------------------------------------------------------

void run_world_reaction_consumes_same_cell_map_input_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    // Put processed wood on SameCellMap (not in inventory)
    auto spawn = world_runtime.spawnEntityOnMap(
        "wood_001", "wood", "entity.wood",
        coord, 2, "wood:processed", true,
        std::vector<std::string>{"processed"}, {}, {});
    assert(spawn.is_ok());

    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    // Wood on map should be consumed (quantity reduced from 2 to 1)
    auto ent_res = world_runtime.findEntity("wood_001");
    assert(ent_res.is_ok());
    assert(ent_res.value()->quantity == 1);

    // Torch should be in inventory
    auto inv_id = inventory_runtime.ensureActorInventory("player");
    auto inv_res = inventory_runtime.findInventory(inv_id.value());
    assert(inv_res.is_ok());
    bool has_torch = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_key == "torch") {
            has_torch = true;
            break;
        }
    }
    assert(has_torch);

    std::cout << "world_reaction_consumes_same_cell_map_input: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Consumes adjacent map input
// ---------------------------------------------------------------------------

void run_world_reaction_consumes_adjacent_map_input_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    // Put processed wood on adjacent cell (not same cell, not inventory)
    WorldCellCoord adj{coord.x + 1, coord.y, coord.layer_key};
    auto spawn = world_runtime.spawnEntityOnMap(
        "wood_001", "wood", "entity.wood",
        adj, 1, "wood:processed", true,
        std::vector<std::string>{"processed"}, {}, {});
    assert(spawn.is_ok());

    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    // Wood on adjacent map should be destroyed (fully consumed)
    auto ent_res = world_runtime.findEntity("wood_001");
    assert(ent_res.is_error());

    std::cout << "world_reaction_consumes_adjacent_map_input: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// DropOnMap output spawns entity on actor cell
// ---------------------------------------------------------------------------

void run_world_reaction_drop_on_map_output_spawns_entity_on_actor_cell_tests() {
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

    auto registry = makeTestContentRegistry(makeTorchReaction());
    WorldReactionService service(*registry, world_runtime, inventory_runtime, world_runtime);

    auto actor_res = world_runtime.findActor("player");
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    addProcessedWoodToInventory(inventory_runtime, "player", 1);
    addCampFireToMap(world_runtime, coord);

    WorldReactionRequest req;
    req.reaction_command_id = "cmd1";
    req.actor_key = "player";
    req.reaction_key = "wood_plus_fire_make_torch";
    req.action_kind = WorldReactionActionKind::Craft;
    req.output_location_policy = WorldReactionOutputLocationPolicy::DropOnMap;

    auto result = service.execute(req);
    assert(result.is_ok());
    assert(result.value().ok);

    // Find torch entity id
    std::string torch_entity_id;
    for (const auto& delta : result.value().state_deltas) {
        if (delta.delta_kind == "item_created" && delta.fields.at("object_key") == "torch") {
            torch_entity_id = delta.target_ref;
            break;
        }
    }
    assert(!torch_entity_id.empty());

    auto ent_res = world_runtime.findEntity(torch_entity_id);
    assert(ent_res.is_ok());
    const auto* entity = ent_res.value();
    assert(entity->location_kind == WorldEntityLocationKind::OnMap);
    assert(entity->coord.has_value());
    assert(entity->coord.value().x == coord.x);
    assert(entity->coord.value().y == coord.y);
    assert(entity->entity_key == "torch");
    assert(entity->quantity == 1);

    std::cout << "world_reaction_drop_on_map_output_spawns_entity_on_actor_cell: passed" << std::endl;
}
