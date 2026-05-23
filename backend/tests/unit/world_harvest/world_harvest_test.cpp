#include "pathfinder/world_harvest/world_harvest_types.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include <cassert>
#include <iostream>
#include <cmath>

using namespace pathfinder::world_harvest;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static WorldGridRuntime* g_world_runtime = nullptr;
static WorldInventoryRuntime* g_inventory_runtime = nullptr;
static ResourceHarvestService* g_service = nullptr;

static void setupTestEnv() {
    static WorldGridRuntime world_runtime;
    static WorldInventoryRuntime inventory_runtime(world_runtime);

    WorldRuntimeConfig config;
    config.seed = 42;
    config.region_size = 16;
    config.initial_region_radius = 0;
    config.default_vision_radius = 3;

    world_runtime.initialize(config);
    world_runtime.generateInitialWorld(config);
    inventory_runtime.initialize();

    g_world_runtime = &world_runtime;
    g_inventory_runtime = &inventory_runtime;
    g_service = new ResourceHarvestService(world_runtime, world_runtime, inventory_runtime);
}

static WorldResourceNodeRuntime makeNode(
    const std::string& node_id,
    const WorldCellCoord& coord,
    const std::string& action_key,
    int charges,
    const std::string& tool_key,
    const std::vector<std::string>& outputs)
{
    WorldResourceNodeRuntime node;
    node.node_id = node_id;
    node.resource_key = "test_resource";
    node.coord = coord;
    node.node_kind_str = "plant";
    node.node_state_str = "Available";
    node.remaining_charges = charges;
    node.max_charges = charges;
    node.required_action_key = action_key;
    node.required_tool_key = tool_key;
    node.output_entity_keys = outputs;
    return node;
}

static void addToolToActorWithNumeric(
    const std::string& actor_key,
    const std::string& tool_key,
    const std::map<std::string, double>& numeric_states) {
    // Spawn tool on map at actor coord, then pickup through the inventory pipeline.
    auto actor_res = g_world_runtime->findActor(actor_key);
    assert(actor_res.is_ok());
    auto coord = actor_res.value()->coord;

    std::string tool_entity_id = actor_key + "_" + tool_key;
    auto spawn_res = g_world_runtime->spawnEntityOnMap(
        tool_entity_id, tool_key, tool_key + "_name", coord,
        1, tool_key + ":default", true, {}, numeric_states, {});
    assert(spawn_res.is_ok());

    InventoryTransferRequest req;
    req.transfer_id = "pickup_" + tool_entity_id;
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = actor_key;
    req.entity_id = tool_entity_id;
    req.quantity = 1;
    auto transfer_res = g_inventory_runtime->transfer(req);
    assert(transfer_res.is_ok());
    assert(transfer_res.value().ok);
}

static void addToolToActor(const std::string& actor_key, const std::string& tool_key) {
    addToolToActorWithNumeric(actor_key, tool_key, {});
}

// ---------------------------------------------------------------------------
// Type tests
// ---------------------------------------------------------------------------

void run_world_harvest_enum_to_string_tests() {
    assert(toString(ResourceHarvestKind::Gather) == "gather");
    assert(toString(ResourceHarvestKind::Chop) == "chop");
    assert(toString(ResourceHarvestKind::Mine) == "mine");
    assert(toString(ResourceHarvestKind::Dig) == "dig");
    assert(toString(ResourceHarvestKind::Unknown) == "unknown");

    assert(toString(ResourceHarvestFailureKind::None) == "none");
    assert(toString(ResourceHarvestFailureKind::ActorMissing) == "actor_missing");
    assert(toString(ResourceHarvestFailureKind::NodeMissing) == "node_missing");
    assert(toString(ResourceHarvestFailureKind::NodeDepleted) == "node_depleted");
    assert(toString(ResourceHarvestFailureKind::ActionMismatch) == "action_mismatch");
    assert(toString(ResourceHarvestFailureKind::ToolMissing) == "tool_missing");

    assert(toString(HarvestOutputLocationPolicy::DropOnMap) == "drop_on_map");
    assert(toString(HarvestOutputLocationPolicy::PreferInventory) == "prefer_inventory");

    std::cout << "world_harvest_enum_to_string: passed" << std::endl;
}

void run_world_harvest_request_validate_basic_tests() {
    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;
    assert(req.validateBasic().is_ok());

    req.harvest_id = "";
    assert(req.validateBasic().is_error());
    req.harvest_id = "h1";

    req.actor_key = "";
    assert(req.validateBasic().is_error());
    req.actor_key = "player";

    req.node_id = "";
    assert(req.validateBasic().is_error());
    req.node_id = "node1";

    req.harvest_kind = ResourceHarvestKind::Unknown;
    assert(req.validateBasic().is_error());

    std::cout << "world_harvest_request_validate_basic: passed" << std::endl;
}

void run_world_harvest_kind_from_command_kind_tests() {
    using pathfinder::world_command::WorldCommandKind;
    assert(harvestKindFromCommandKind(WorldCommandKind::Gather) == ResourceHarvestKind::Gather);
    assert(harvestKindFromCommandKind(WorldCommandKind::Chop) == ResourceHarvestKind::Chop);
    assert(harvestKindFromCommandKind(WorldCommandKind::Mine) == ResourceHarvestKind::Mine);
    assert(harvestKindFromCommandKind(WorldCommandKind::Dig) == ResourceHarvestKind::Dig);
    assert(harvestKindFromCommandKind(WorldCommandKind::Unknown) == ResourceHarvestKind::Unknown);

    std::cout << "world_harvest_kind_from_command_kind: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate tests
// ---------------------------------------------------------------------------

void run_world_harvest_validate_actor_missing_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "nonexistent_actor";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::ActorMissing);

    std::cout << "world_harvest_validate_actor_missing: passed" << std::endl;
}

void run_world_harvest_validate_node_missing_tests() {
    setupTestEnv();
    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "nonexistent_node";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::NodeMissing);

    std::cout << "world_harvest_validate_node_missing: passed" << std::endl;
}

void run_world_harvest_validate_node_depleted_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 0, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::NodeDepleted);

    std::cout << "world_harvest_validate_node_depleted: passed" << std::endl;
}

void run_world_harvest_validate_action_mismatch_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop; // mismatch

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::ActionMismatch);

    std::cout << "world_harvest_validate_action_mismatch: passed" << std::endl;
}

void run_world_harvest_validate_too_far_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 2, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::NodeTooFar);

    std::cout << "world_harvest_validate_too_far: passed" << std::endl;
}

void run_world_harvest_validate_not_visible_tests() {
    setupTestEnv();
    // Place node outside vision radius (player at 0,0 with radius 3)
    auto node = makeNode("node1", WorldCellCoord{0, 4, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::NodeNotVisible);

    std::cout << "world_harvest_validate_not_visible: passed" << std::endl;
}

void run_world_harvest_validate_tool_missing_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "chop", 3, "stone_axe", {"wood"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::ToolMissing);

    std::cout << "world_harvest_validate_tool_missing: passed" << std::endl;
}

void run_world_harvest_validate_tool_present_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "chop", 3, "stone_axe", {"wood"});
    g_world_runtime->upsertGeneratedResourceNode(node);
    addToolToActor("player", "stone_axe");

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop;

    auto result = g_service->execute(req);
    assert(result.ok);

    std::cout << "world_harvest_validate_tool_present: passed" << std::endl;
}

void run_world_harvest_tool_durability_decrements_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "chop", 3, "stone_axe", {"wood"});
    g_world_runtime->upsertGeneratedResourceNode(node);
    addToolToActorWithNumeric("player", "stone_axe", {{"durability", 2}, {"max_durability", 2}});

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop;

    auto result = g_service->execute(req);
    assert(result.ok);

    InventoryOwnerRef owner{InventoryOwnerKind::Actor, "player"};
    auto tools = g_inventory_runtime->queryItems(owner, "stone_axe").value();
    assert(tools.size() == 1);
    assert(tools[0].numeric_states.at("durability") == 1);

    std::cout << "world_harvest_tool_durability_decrements: passed" << std::endl;
}

void run_world_harvest_worn_out_tool_rejected_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "chop", 3, "stone_axe", {"wood"});
    g_world_runtime->upsertGeneratedResourceNode(node);
    addToolToActorWithNumeric("player", "stone_axe", {{"durability", 0}, {"max_durability", 2}});

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::ToolMissing);

    std::cout << "world_harvest_worn_out_tool_rejected: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Apply tests
// ---------------------------------------------------------------------------

void run_world_harvest_apply_gather_creates_output_on_map_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);
    assert(!result.changed_entity_ids.empty());

    auto entity_res = g_world_runtime->findEntity(result.changed_entity_ids[0]);
    assert(entity_res.is_ok());
    assert(entity_res.value()->location_kind == WorldEntityLocationKind::OnMap);
    assert(entity_res.value()->entity_key == "red_berry");

    std::cout << "world_harvest_apply_gather_creates_output_on_map: passed" << std::endl;
}

void run_world_harvest_apply_chop_creates_wood_on_map_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "chop", 3, "stone_axe", {"wood"});
    g_world_runtime->upsertGeneratedResourceNode(node);
    addToolToActor("player", "stone_axe");

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop;

    auto result = g_service->execute(req);
    assert(result.ok);
    assert(!result.changed_entity_ids.empty());

    auto entity_res = g_world_runtime->findEntity(result.changed_entity_ids[0]);
    assert(entity_res.is_ok());
    assert(entity_res.value()->entity_key == "wood");

    std::cout << "world_harvest_apply_chop_creates_wood_on_map: passed" << std::endl;
}

void run_world_harvest_apply_consumes_one_charge_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    assert(before == 3);

    auto result = g_service->execute(req);
    assert(result.ok);

    auto after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    assert(after == 2);

    std::cout << "world_harvest_apply_consumes_one_charge: passed" << std::endl;
}

void run_world_harvest_apply_depletes_node_at_zero_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 1, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);

    auto node_after = g_world_runtime->findResourceNode("node1").value();
    assert(node_after->remaining_charges == 0);
    assert(node_after->node_state_str == "Depleted");

    std::cout << "world_harvest_apply_depletes_node_at_zero: passed" << std::endl;
}

void run_world_harvest_apply_output_id_deterministic_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);
    assert(result.changed_entity_ids[0] == "h1_red_berry_0");

    std::cout << "world_harvest_apply_output_id_deterministic: passed" << std::endl;
}

void run_world_harvest_apply_failure_does_not_change_node_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop; // mismatch -> fail

    auto before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    auto result = g_service->execute(req);
    assert(!result.ok);
    auto after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    assert(before == after);

    std::cout << "world_harvest_apply_failure_does_not_change_node: passed" << std::endl;
}

void run_world_harvest_apply_failure_does_not_spawn_output_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int entity_count_before = 0;
    {
        auto snap = g_world_runtime->snapshotForDebug().value();
        entity_count_before = static_cast<int>(snap.entities.size());
    }

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Chop; // mismatch -> fail

    auto result = g_service->execute(req);
    assert(!result.ok);

    int entity_count_after = 0;
    {
        auto snap = g_world_runtime->snapshotForDebug().value();
        entity_count_after = static_cast<int>(snap.entities.size());
    }
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_apply_failure_does_not_spawn_output: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// P45 ownership tests
// ---------------------------------------------------------------------------

void run_world_harvest_output_uses_p45_on_map_location_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);

    auto entity_res = g_world_runtime->findEntity(result.changed_entity_ids[0]);
    assert(entity_res.is_ok());
    assert(entity_res.value()->location_kind == WorldEntityLocationKind::OnMap);
    assert(entity_res.value()->coord.has_value());

    std::cout << "world_harvest_output_uses_p45_on_map_location: passed" << std::endl;
}

void run_world_harvest_output_not_in_actor_inventory_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);

    // Ensure actor inventory does not contain the output
    InventoryOwnerRef owner;
    owner.owner_kind = InventoryOwnerKind::Actor;
    owner.owner_key = "player";
    auto items = g_inventory_runtime->queryItems(owner, "red_berry").value();
    assert(items.empty());

    std::cout << "world_harvest_output_not_in_actor_inventory: passed" << std::endl;
}

void run_world_harvest_output_cell_contains_entity_id_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);

    auto entity_res = g_world_runtime->findEntity(result.changed_entity_ids[0]);
    assert(entity_res.is_ok());
    assert(entity_res.value()->coord.has_value());

    auto cell_res = g_world_runtime->findCell(*entity_res.value()->coord);
    assert(cell_res.is_ok());
    const auto* cell = cell_res.value();
    bool found = false;
    for (const auto& eid : cell->entity_ids) {
        if (eid == result.changed_entity_ids[0]) {
            found = true;
            break;
        }
    }
    assert(found);

    const auto& output_coord = *entity_res.value()->coord;
    assert(std::abs(output_coord.x - node.coord.x) <= 1);
    assert(std::abs(output_coord.y - node.coord.y) <= 1);

    std::cout << "world_harvest_output_cell_contains_entity_id: passed" << std::endl;
}

void run_world_harvest_output_can_be_picked_up_by_p45_pickup_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(result.ok);
    std::string output_id = result.changed_entity_ids[0];
    auto entity_res = g_world_runtime->findEntity(output_id);
    assert(entity_res.is_ok());
    assert(entity_res.value()->coord.has_value());
    auto move_res = g_world_runtime->moveActor("player", node.coord);
    assert(move_res.is_ok());
    assert(move_res.value().moved);
    const auto output_coord = *entity_res.value()->coord;
    const int output_dx = std::abs(output_coord.x - node.coord.x);
    const int output_dy = std::abs(output_coord.y - node.coord.y);
    if (output_dx == 1 && output_dy == 1) {
        WorldCellCoord pickup_coord{output_coord.x, node.coord.y, node.coord.layer_key};
        auto bridge_move_res = g_world_runtime->moveActor("player", pickup_coord);
        assert(bridge_move_res.is_ok());
        assert(bridge_move_res.value().moved);
    }

    // Pickup via P45
    InventoryTransferRequest pickup_req;
    pickup_req.transfer_id = "pickup_1";
    pickup_req.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup_req.actor_key = "player";
    pickup_req.entity_id = output_id;
    pickup_req.quantity = 1;

    auto transfer_res = g_inventory_runtime->transfer(pickup_req);
    assert(transfer_res.is_ok());
    assert(transfer_res.value().ok);

    // Verify in inventory
    InventoryOwnerRef owner;
    owner.owner_kind = InventoryOwnerKind::Actor;
    owner.owner_key = "player";
    auto items = g_inventory_runtime->queryItems(owner, "red_berry").value();
    assert(!items.empty());
    assert(items[0].quantity == 1);

    std::cout << "world_harvest_output_can_be_picked_up_by_p45_pickup: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Audit-required tests
// ---------------------------------------------------------------------------

void run_world_harvest_rejects_prefer_inventory_policy_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int charges_before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_before = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;
    req.output_location_policy = HarvestOutputLocationPolicy::PreferInventory;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::InvalidRequest);

    // Must not consume charge or spawn output
    int charges_after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_after = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());
    assert(charges_before == charges_after);
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_rejects_prefer_inventory_policy: passed" << std::endl;
}

void run_world_harvest_rejects_empty_outputs_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int charges_before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_before = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::OutputInvalid);

    int charges_after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_after = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());
    assert(charges_before == charges_after);
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_rejects_empty_outputs: passed" << std::endl;
}

void run_world_harvest_rejects_empty_output_key_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry", ""});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int charges_before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_before = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::OutputInvalid);

    int charges_after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_after = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());
    assert(charges_before == charges_after);
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_rejects_empty_output_key: passed" << std::endl;
}

void run_world_harvest_rejects_unknown_policy_string_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int charges_before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_before = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());

    ResourceHarvestRequest req;
    req.harvest_id = "h1";
    req.actor_key = "player";
    req.node_id = "node1";
    req.harvest_kind = ResourceHarvestKind::Gather;
    req.output_location_policy = HarvestOutputLocationPolicy::Unknown;

    auto result = g_service->execute(req);
    assert(!result.ok);
    assert(result.failure_kind == ResourceHarvestFailureKind::InvalidRequest);

    int charges_after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_after = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());
    assert(charges_before == charges_after);
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_rejects_unknown_policy_string: passed" << std::endl;
}

void run_world_harvest_command_rejects_bad_policy_parameter_tests() {
    setupTestEnv();
    auto node = makeNode("node1", WorldCellCoord{0, 1, "surface"}, "gather", 3, "", {"red_berry"});
    g_world_runtime->upsertGeneratedResourceNode(node);

    int charges_before = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_before = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());

    // Construct request via command handler path: bad_policy -> Unknown -> rejected
    pathfinder::world_command::WorldCommandDto cmd;
    cmd.command_id = "cmd_bad";
    cmd.command_kind = pathfinder::world_command::WorldCommandKind::Gather;
    cmd.actor_key = "player";
    cmd.target.target_entity_id = "node1";
    cmd.parameters["output_location_policy"] = "bad_policy";

    pathfinder::world_command::WorldCommandContext context(cmd);
    context.setCurrentTick(1);

    auto handler = pathfinder::world_command::createGatherCommandHandler(*g_service);
    auto result = handler->execute(context, cmd);
    assert(result.is_ok());
    assert(result.value().result_kind == pathfinder::world_command::WorldCommandResultKind::Failed);
    assert(!result.value().failure_reason_keys.empty());

    int charges_after = g_world_runtime->findResourceNode("node1").value()->remaining_charges;
    int entity_count_after = static_cast<int>(g_world_runtime->snapshotForDebug().value().entities.size());
    assert(charges_before == charges_after);
    assert(entity_count_before == entity_count_after);

    std::cout << "world_harvest_command_rejects_bad_policy_parameter: passed" << std::endl;
}
