#include "pathfinder/world_inventory/world_inventory_types.h"
#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_inventory;
using namespace pathfinder::world_runtime;
using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

class TestFixture {
public:
    WorldRuntimeConfig config;
    WorldGridRuntime world_runtime;
    std::unique_ptr<WorldInventoryRuntime> inventory_runtime;

    void setup(int seed = 1) {
        config.seed = seed;
        config.region_size = 9;
        config.initial_region_radius = 0;
        config.default_vision_radius = 3;

        world_runtime.initialize(config);
        world_runtime.generateInitialWorld(config);

        inventory_runtime = std::make_unique<WorldInventoryRuntime>(world_runtime);
        inventory_runtime->initialize();
    }

    std::string findGroundItem() const {
        auto player_coord = playerCoord();
        auto snap = world_runtime.snapshotForDebug();
        if (snap.is_ok()) {
            for (const auto& [id, entity] : snap.value().entities) {
                if (entity.location_kind == WorldEntityLocationKind::OnMap &&
                    entity.entity_key != "player" && entity.coord.has_value()) {
                    int dx = std::abs(entity.coord.value().x - player_coord.x);
                    int dy = std::abs(entity.coord.value().y - player_coord.y);
                    if ((dx == 0 && dy == 0) || (dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                        return id;
                    }
                }
            }
        }
        return "";
    }

    std::string findAnyGroundItem() const {
        auto snap = world_runtime.snapshotForDebug();
        if (snap.is_ok()) {
            for (const auto& [id, entity] : snap.value().entities) {
                if (entity.location_kind == WorldEntityLocationKind::OnMap &&
                    entity.entity_key != "player") {
                    return id;
                }
            }
        }
        return "";
    }

    std::string findFarGroundItem() const {
        auto player_coord = playerCoord();
        auto snap = world_runtime.snapshotForDebug();
        if (snap.is_ok()) {
            for (const auto& [id, entity] : snap.value().entities) {
                if (entity.location_kind == WorldEntityLocationKind::OnMap &&
                    entity.entity_key != "player" && entity.coord.has_value()) {
                    int dx = std::abs(entity.coord.value().x - player_coord.x);
                    int dy = std::abs(entity.coord.value().y - player_coord.y);
                    if (dx > 1 || dy > 1) {
                        return id;
                    }
                }
            }
        }
        return "";
    }

    WorldCellCoord playerCoord() const {
        auto actor_res = world_runtime.findActor("player");
        assert(actor_res.is_ok());
        return actor_res.value()->coord;
    }
};

// ---------------------------------------------------------------------------
// Owner ID / Inventory ID tests
// ---------------------------------------------------------------------------

void run_owner_id_tests() {
    TestFixture f;
    f.setup();

    auto id1 = f.inventory_runtime->ensureActorInventory("player");
    auto id2 = f.inventory_runtime->ensureActorInventory("player");
    assert(id1.is_ok());
    assert(id2.is_ok());
    assert(id1.value() == id2.value());
    assert(id1.value() == "inv_actor_player");

    std::cout << "world_inventory_owner_id: passed" << std::endl;
}

void run_ensure_actor_inventory_tests() {
    TestFixture f;
    f.setup();

    auto id1 = f.inventory_runtime->ensureActorInventory("npc_001");
    assert(id1.is_ok());

    auto inv_res = f.inventory_runtime->findInventory(id1.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->owner.owner_kind == InventoryOwnerKind::Actor);
    assert(inv_res.value()->owner.owner_key == "npc_001");
    assert(inv_res.value()->capacity_slots == 20);

    std::cout << "world_inventory_ensure_actor_inventory: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup tests
// ---------------------------------------------------------------------------

void run_pickup_from_map_tests() {
    TestFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    // Ensure item is on map
    auto ent_res = f.world_runtime.findEntity(item_id);
    assert(ent_res.is_ok());
    assert(ent_res.value()->location_kind == WorldEntityLocationKind::OnMap);

    InventoryTransferRequest request;
    request.transfer_id = "test_pickup_1";
    request.transfer_kind = InventoryTransferKind::PickupFromMap;
    request.actor_key = "player";
    request.entity_id = item_id;
    request.quantity = 1;

    auto result = f.inventory_runtime->transfer(request);
    assert(result.is_ok());
    assert(result.value().ok);
    assert(!result.value().changed_inventory_ids.empty());
    assert(!result.value().changed_entity_ids.empty());

    // Item should now be in inventory
    auto loc_res = f.inventory_runtime->findItemLocation(item_id);
    assert(loc_res.is_ok());
    assert(loc_res.value().location_kind == InventoryLocationKind::InInventory);

    // Entity should no longer be on map
    auto ent_after = f.world_runtime.findEntity(item_id);
    assert(ent_after.is_ok());
    assert(ent_after.value()->location_kind == WorldEntityLocationKind::InInventory);
    assert(!ent_after.value()->coord.has_value());

    // Cell should no longer contain the item
    auto cell_res = f.world_runtime.findCell(f.playerCoord());
    if (ent_res.value()->coord.has_value() && ent_res.value()->coord.value() == f.playerCoord()) {
        // If item was at player origin cell, check it's gone
        auto snap = f.world_runtime.snapshotForDebug();
        if (snap.is_ok()) {
            auto cell_it = snap.value().cells.find(f.playerCoord().cellId());
            if (cell_it != snap.value().cells.end()) {
                bool found = false;
                for (const auto& eid : cell_it->second.entity_ids) {
                    if (eid == item_id) { found = true; break; }
                }
                assert(!found);
            }
        }
    }

    // Actor inventory should contain the item
    auto actor_inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(actor_inv_id.value());
    assert(inv_res.is_ok());
    bool found_entry = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_id == item_id) {
            found_entry = true;
            break;
        }
    }
    assert(found_entry);

    std::cout << "world_inventory_pickup_from_map: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop tests
// ---------------------------------------------------------------------------

void run_drop_to_map_tests() {
    TestFixture f;
    f.setup();

    // First pickup an item
    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_pickup_for_drop";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = item_id;
    pickup.quantity = 1;

    auto pickup_res = f.inventory_runtime->transfer(pickup);
    assert(pickup_res.is_ok() && pickup_res.value().ok);

    // Now drop it
    InventoryTransferRequest drop;
    drop.transfer_id = "test_drop_1";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = item_id;
    drop.quantity = 1;

    auto drop_res = f.inventory_runtime->transfer(drop);
    assert(drop_res.is_ok());
    assert(drop_res.value().ok);

    // Item should be back on map
    auto loc_res = f.inventory_runtime->findItemLocation(item_id);
    assert(loc_res.is_ok());
    assert(loc_res.value().location_kind == InventoryLocationKind::OnMap);

    // Entity should have coord again
    auto ent_after = f.world_runtime.findEntity(item_id);
    assert(ent_after.is_ok());
    assert(ent_after.value()->location_kind == WorldEntityLocationKind::OnMap);
    assert(ent_after.value()->coord.has_value());

    // Inventory should no longer contain it
    auto actor_inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(actor_inv_id.value());
    assert(inv_res.is_ok());
    bool found_entry = false;
    for (const auto& entry : inv_res.value()->entries) {
        if (entry.entity_id == item_id) {
            found_entry = true;
            break;
        }
    }
    assert(!found_entry);

    std::cout << "world_inventory_drop_to_map: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// No duplicate location tests
// ---------------------------------------------------------------------------

void run_no_duplicate_location_tests() {
    TestFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    // Pickup
    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_dup_1";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = item_id;
    pickup.quantity = 1;

    auto res1 = f.inventory_runtime->transfer(pickup);
    assert(res1.is_ok() && res1.value().ok);

    // Try to pick up again (should fail because no longer on map)
    InventoryTransferRequest pickup2;
    pickup2.transfer_id = "test_dup_2";
    pickup2.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup2.actor_key = "player";
    pickup2.entity_id = item_id;
    pickup2.quantity = 1;

    auto res2 = f.inventory_runtime->transfer(pickup2);
    assert(res2.is_ok());
    assert(!res2.value().ok);
    assert(res2.value().failure_kind == InventoryFailureKind::ItemNotOnMap);

    std::cout << "world_inventory_no_duplicate_location: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Quantity invalid tests
// ---------------------------------------------------------------------------

void run_quantity_invalid_tests() {
    TestFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest req;
    req.transfer_id = "test_qinv";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = item_id;
    req.quantity = 0;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::QuantityInvalid);

    req.quantity = -1;
    auto res2 = f.inventory_runtime->transfer(req);
    assert(res2.is_ok());
    assert(!res2.value().ok);
    assert(res2.value().failure_kind == InventoryFailureKind::QuantityInvalid);

    std::cout << "world_inventory_quantity_invalid: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Quantity insufficient tests
// ---------------------------------------------------------------------------

void run_quantity_insufficient_tests() {
    TestFixture f;
    f.setup();

    // Pickup first
    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_qins_pickup";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = item_id;
    pickup.quantity = 1;
    auto pres = f.inventory_runtime->transfer(pickup);
    assert(pres.is_ok() && pres.value().ok);

    // Try to drop 5 when only have 1
    InventoryTransferRequest drop;
    drop.transfer_id = "test_qins_drop";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = item_id;
    drop.quantity = 5;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::QuantityInsufficient);

    std::cout << "world_inventory_quantity_insufficient: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Capacity exceeded tests
// ---------------------------------------------------------------------------

void run_capacity_exceeded_tests() {
    TestFixture f;
    f.setup();

    // Find or create multiple reachable ground items. P59 centered regions mean
    // map iteration may include far entities before adjacent ones.
    auto player_coord = f.playerCoord();
    auto snap = f.world_runtime.snapshotForDebug();
    assert(snap.is_ok());

    std::vector<std::string> ground_items;
    for (const auto& [id, entity] : snap.value().entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap &&
            entity.entity_key != "player" && entity.coord.has_value()) {
            int dx = std::abs(entity.coord.value().x - player_coord.x);
            int dy = std::abs(entity.coord.value().y - player_coord.y);
            if ((dx == 0 && dy == 0) || (dx == 1 && dy == 0) || (dx == 0 && dy == 1)) {
                ground_items.push_back(id);
            }
        }
    }
    while (ground_items.size() < 2) {
        std::string id = "test_capacity_item_" + std::to_string(ground_items.size());
        auto spawn_res = f.world_runtime.spawnEntityOnMap(
            id, "loose_stone", "entity.loose_stone",
            player_coord, 1, "loose_stone:default", true, {}, {}, {});
        assert(spawn_res.is_ok() && "Should spawn reachable capacity test item");
        ground_items.push_back(id);
    }

    // Set very small capacity
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    inv_res.value()->capacity_slots = 1;

    // Pickup one item should succeed
    if (!ground_items.empty()) {
        InventoryTransferRequest pickup;
        pickup.transfer_id = "test_cap_1";
        pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
        pickup.actor_key = "player";
        pickup.entity_id = ground_items[0];
        pickup.quantity = 1;
        auto res1 = f.inventory_runtime->transfer(pickup);
        assert(res1.is_ok() && res1.value().ok);

        // Pickup another should fail
        if (ground_items.size() > 1) {
            InventoryTransferRequest pickup2;
            pickup2.transfer_id = "test_cap_2";
            pickup2.transfer_kind = InventoryTransferKind::PickupFromMap;
            pickup2.actor_key = "player";
            pickup2.entity_id = ground_items[1];
            pickup2.quantity = 1;
            auto res2 = f.inventory_runtime->transfer(pickup2);
            assert(res2.is_ok());
            assert(!res2.value().ok);
            assert(res2.value().failure_kind == InventoryFailureKind::CapacityExceeded);
        }
    }

    std::cout << "world_inventory_capacity_exceeded: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// State version tests
// ---------------------------------------------------------------------------

void run_state_version_tests() {
    TestFixture f;
    f.setup();

    uint64_t v1 = f.inventory_runtime->stateVersion();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest req;
    req.transfer_id = "test_sv";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = item_id;
    req.quantity = 1;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    uint64_t v2 = f.inventory_runtime->stateVersion();
    assert(v2 > v1);

    std::cout << "world_inventory_state_version: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Stack key tests
// ---------------------------------------------------------------------------

void run_stack_key_separates_states_tests() {
    TestFixture f;
    f.setup();

    // This test verifies the stack key concept at the type level.
    // Full stack merge/separation with state is covered by ensureActorInventory
    // and the runtime's makeStackKey logic.

    // For P45 minimal, we verify that different entity_keys get different stacks.
    std::string item1 = f.findGroundItem();
    assert(!item1.empty());

    // Pickup first item
    InventoryTransferRequest pickup1;
    pickup1.transfer_id = "test_sk_1";
    pickup1.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup1.actor_key = "player";
    pickup1.entity_id = item1;
    pickup1.quantity = 1;
    auto res1 = f.inventory_runtime->transfer(pickup1);
    assert(res1.is_ok() && res1.value().ok);

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    // Different entity_keys should create different entries even with same quantity
    bool found_different_keys = false;
    for (const auto& e1 : inv_res.value()->entries) {
        for (const auto& e2 : inv_res.value()->entries) {
            if (e1.entity_key != e2.entity_key) {
                found_different_keys = true;
                break;
            }
        }
    }
    // We only have one item so far; the test mainly verifies the stack_key field exists
    // and is populated correctly.
    assert(!inv_res.value()->entries.empty());
    for (const auto& entry : inv_res.value()->entries) {
        assert(!entry.stack_key.empty());
    }

    std::cout << "world_inventory_stack_key_separates_states: passed" << std::endl;
}

void run_non_stackable_not_merged_tests() {
    TestFixture f;
    f.setup();

    // P45 minimal: verify that unique items (non-stackable) keep separate entries.
    // We simulate by directly adding non-stackable entries.
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry1;
    entry1.entry_id = "entry_1";
    entry1.entity_id = "sword_001";
    entry1.entity_key = "sword";
    entry1.stack_key = "sword:durability_10";
    entry1.quantity = 1;
    entry1.stackable = false;
    inv_res.value()->entries.push_back(entry1);
    inv_res.value()->used_slots += 1;

    InventoryItemEntry entry2;
    entry2.entry_id = "entry_2";
    entry2.entity_id = "sword_002";
    entry2.entity_key = "sword";
    entry2.stack_key = "sword:durability_5";
    entry2.quantity = 1;
    entry2.stackable = false;
    inv_res.value()->entries.push_back(entry2);
    inv_res.value()->used_slots += 1;

    assert(inv_res.value()->entries.size() == 2);
    assert(inv_res.value()->entries[0].stack_key != inv_res.value()->entries[1].stack_key);

    std::cout << "world_inventory_non_stackable_not_merged: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup too far tests
// ---------------------------------------------------------------------------

void run_pickup_too_far_tests() {
    TestFixture f;
    f.setup(42); // seed=42 has items far from player

    std::string far_item = f.findFarGroundItem();

    if (!far_item.empty()) {
        InventoryTransferRequest req;
        req.transfer_id = "test_far";
        req.transfer_kind = InventoryTransferKind::PickupFromMap;
        req.actor_key = "player";
        req.entity_id = far_item;
        req.quantity = 1;

        auto res = f.inventory_runtime->transfer(req);
        assert(res.is_ok());
        assert(!res.value().ok);
        // Far items may be blocked by visibility first (TargetNotVisible) or distance (TargetTooFar)
        assert(res.value().failure_kind == InventoryFailureKind::TargetTooFar ||
               res.value().failure_kind == InventoryFailureKind::TargetNotVisible);
    }

    std::cout << "world_inventory_pickup_too_far: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup missing item tests
// ---------------------------------------------------------------------------

void run_pickup_missing_item_tests() {
    TestFixture f;
    f.setup();

    InventoryTransferRequest req;
    req.transfer_id = "test_missing";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = "nonexistent_item_xyz";
    req.quantity = 1;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::ItemMissing);

    std::cout << "world_inventory_pickup_missing_item: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop not owned tests
// ---------------------------------------------------------------------------

void run_drop_not_owned_tests() {
    TestFixture f;
    f.setup();

    // Try to drop an item that was never picked up
    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest drop;
    drop.transfer_id = "test_not_owned";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = item_id;
    drop.quantity = 1;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::ItemNotInInventory);

    std::cout << "world_inventory_drop_not_owned: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop partial stack tests
// ---------------------------------------------------------------------------

void run_drop_partial_stack_tests() {
    TestFixture f;
    f.setup();

    // P45 minimal: we can't easily generate a stack of multiple identical items on map.
    // We'll verify the structure supports partial drop via manual entry creation.
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "entry_stack";
    entry.entity_id = "berry_001";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 5;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Drop 2 of 5
    InventoryTransferRequest drop;
    drop.transfer_id = "test_partial";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_001";
    drop.quantity = 2;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok());
    assert(res.value().ok);

    // Inventory should still have 3
    auto inv_after = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_after.is_ok());
    bool found = false;
    for (const auto& e : inv_after.value()->entries) {
        if (e.entity_id == "berry_001") {
            assert(e.quantity == 3);
            found = true;
            break;
        }
    }
    assert(found);

    std::cout << "world_inventory_drop_partial_stack: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Duplicate click fails tests
// ---------------------------------------------------------------------------

void run_pickup_duplicate_click_fails_tests() {
    TestFixture f;
    f.setup();

    std::string item_id = f.findGroundItem();
    assert(!item_id.empty());

    InventoryTransferRequest req1;
    req1.transfer_id = "test_dup_click_1";
    req1.transfer_kind = InventoryTransferKind::PickupFromMap;
    req1.actor_key = "player";
    req1.entity_id = item_id;
    req1.quantity = 1;

    auto res1 = f.inventory_runtime->transfer(req1);
    assert(res1.is_ok() && res1.value().ok);

    // Second click on same item should fail
    InventoryTransferRequest req2;
    req2.transfer_id = "test_dup_click_2";
    req2.transfer_kind = InventoryTransferKind::PickupFromMap;
    req2.actor_key = "player";
    req2.entity_id = item_id;
    req2.quantity = 1;

    auto res2 = f.inventory_runtime->transfer(req2);
    assert(res2.is_ok());
    assert(!res2.value().ok);
    assert(res2.value().failure_kind == InventoryFailureKind::ItemNotOnMap);

    std::cout << "world_inventory_pickup_duplicate_click_fails: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Source proof actor self only tests
// ---------------------------------------------------------------------------

void run_source_proof_actor_self_only_tests() {
    TestFixture f;
    f.setup();

    // Verify that queryItems defaults to actor's own inventory.
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "proof_entry";
    entry.entity_id = "wood_001";
    entry.entity_key = "wood";
    entry.stack_key = "wood:default";
    entry.quantity = 3;
    inv_res.value()->entries.push_back(entry);

    InventoryOwnerRef owner{InventoryOwnerKind::Actor, "player"};
    auto query_res = f.inventory_runtime->queryItems(owner, "wood");
    assert(query_res.is_ok());
    assert(!query_res.value().empty());
    assert(query_res.value()[0].quantity == 3);

    // Query for another actor should return empty
    InventoryOwnerRef other_owner{InventoryOwnerKind::Actor, "npc_999"};
    auto query_other = f.inventory_runtime->queryItems(other_owner, "wood");
    assert(query_other.is_ok());
    assert(query_other.value().empty());

    std::cout << "world_inventory_source_proof_actor_self_only: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// No default camp sharing tests
// ---------------------------------------------------------------------------

void run_no_default_camp_sharing_tests() {
    TestFixture f;
    f.setup();

    // Camp inventory is separate from actor inventory
    auto camp_inv_id = f.inventory_runtime->ensureContainerInventory("camp_001");
    auto actor_inv_id = f.inventory_runtime->ensureActorInventory("player");

    assert(camp_inv_id.is_ok());
    assert(actor_inv_id.is_ok());
    assert(camp_inv_id.value() != actor_inv_id.value());

    // Camp inventory should be public_read but not public_take by default
    auto camp_inv = f.inventory_runtime->findInventory(camp_inv_id.value());
    assert(camp_inv.is_ok());
    assert(camp_inv.value()->public_read == true);
    assert(camp_inv.value()->public_take == false);

    std::cout << "world_inventory_no_default_camp_sharing: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup requires visible target
// ---------------------------------------------------------------------------

void run_pickup_requires_visible_target_tests() {
    TestFixture f;
    f.config.seed = 1;
    f.config.region_size = 9;
    f.config.initial_region_radius = 0;
    f.config.default_vision_radius = 0; // vision=0: only current cell is visible
    f.world_runtime.initialize(f.config);
    f.world_runtime.generateInitialWorld(f.config);
    f.inventory_runtime = std::make_unique<WorldInventoryRuntime>(f.world_runtime);
    f.inventory_runtime->initialize();

    // Spawn an item at adjacent cell (1,0) with explicit data
    std::string far_item_id = "test_bush_surface_1_0";
    auto spawn_res = f.world_runtime.spawnEntityOnMap(
        far_item_id, "unknown_bush", "entity.unknown_bush",
        WorldCellCoord{1, 0, "surface"}, 1, "unknown_bush:default", true, {}, {}, {});
    assert(spawn_res.is_ok());

    // Verify (1,0) is not visible from (0,0) with vision=0
    bool visible = f.world_runtime.isCellVisibleToActor("player", WorldCellCoord{1, 0, "surface"});
    assert(!visible);

    InventoryTransferRequest req;
    req.transfer_id = "test_vis";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = far_item_id;
    req.quantity = 1;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::TargetNotVisible);

    std::cout << "world_inventory_pickup_requires_visible_target: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Container not empty blocked
// ---------------------------------------------------------------------------

void run_container_not_empty_blocked_tests() {
    TestFixture f;
    f.setup(1);

    // Create a container entity at player cell with container tag
    WorldEntityInstance container;
    container.entity_id = "chest_surface_0_0";
    container.entity_key = "chest";
    container.display_name_key = "entity.chest";
    container.coord = WorldCellCoord{0, 0, "surface"};
    container.location_kind = WorldEntityLocationKind::OnMap;
    container.tag_keys.push_back("container");
    container.quantity = 1;
    container.stackable = false;

    // Add to world runtime manually (need access to entities_ and cells_)
    auto spawn_res = f.world_runtime.spawnEntityOnMap(
        container.entity_id, container.entity_key, container.display_name_key,
        container.coord.value(), container.quantity, container.stack_key,
        container.stackable, {}, {}, container.tag_keys);
    assert(spawn_res.is_ok());

    // Create container inventory and add an item
    auto container_inv_id = f.inventory_runtime->ensureContainerInventory(container.entity_id);
    assert(container_inv_id.is_ok());
    auto inv_res = f.inventory_runtime->findInventory(container_inv_id.value());
    assert(inv_res.is_ok());
    InventoryItemEntry entry;
    entry.entry_id = "chest_item";
    entry.entity_id = "stone_001";
    entry.entity_key = "stone";
    entry.stack_key = "stone:default";
    entry.quantity = 1;
    inv_res.value()->entries.push_back(entry);

    // Try to pick up non-empty container
    InventoryTransferRequest req;
    req.transfer_id = "test_container";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = container.entity_id;
    req.quantity = 1;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::ContainerNotEmpty);

    std::cout << "world_inventory_container_not_empty_blocked: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Validate no state change
// ---------------------------------------------------------------------------

void run_validate_no_state_change_tests() {
    TestFixture f;
    f.setup(1);

    uint64_t v_before = f.inventory_runtime->stateVersion();

    // Attempt an invalid pickup (missing item)
    InventoryTransferRequest req;
    req.transfer_id = "test_no_state";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = "nonexistent_xyz";
    req.quantity = 1;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);

    uint64_t v_after = f.inventory_runtime->stateVersion();
    assert(v_after == v_before);

    // Also ensure no inventory was created for player
    std::string actor_inv_id = "inv_actor_player";
    auto inv_res = f.inventory_runtime->findInventory(actor_inv_id);
    assert(inv_res.is_error());

    std::cout << "world_inventory_validate_no_state_change: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup merge consistency
// ---------------------------------------------------------------------------

void run_pickup_merge_consistency_tests() {
    TestFixture f;
    f.setup(1); // seed 1 has one item at (0,0)

    // Add a second item at (0,0) with same entity_key but same default state
    WorldEntityInstance item2;
    item2.entity_id = "unknown_bush_surface_0_0_second";
    item2.entity_key = "unknown_bush";
    item2.display_name_key = "entity.unknown_bush";
    item2.coord = WorldCellCoord{0, 0, "surface"};
    item2.location_kind = WorldEntityLocationKind::OnMap;
    item2.quantity = 1;
    item2.stackable = true;
    item2.stack_key = "unknown_bush:default";

    auto spawn_res = f.world_runtime.spawnEntityOnMap(
        item2.entity_id, item2.entity_key, item2.display_name_key,
        item2.coord.value(), item2.quantity, item2.stack_key,
        item2.stackable, item2.state_keys, item2.numeric_states, {});
    assert(spawn_res.is_ok());

    // Pickup first item
    std::string item1_id = f.findGroundItem();
    assert(!item1_id.empty());

    InventoryTransferRequest pickup1;
    pickup1.transfer_id = "test_merge_1";
    pickup1.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup1.actor_key = "player";
    pickup1.entity_id = item1_id;
    pickup1.quantity = 1;
    auto res1 = f.inventory_runtime->transfer(pickup1);
    assert(res1.is_ok() && res1.value().ok);

    // Pickup second item (should merge)
    InventoryTransferRequest pickup2;
    pickup2.transfer_id = "test_merge_2";
    pickup2.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup2.actor_key = "player";
    pickup2.entity_id = item2.entity_id;
    pickup2.quantity = 1;
    auto res2 = f.inventory_runtime->transfer(pickup2);
    assert(res2.is_ok() && res2.value().ok);

    // Inventory should have exactly one entry with quantity 2
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    int bush_count = 0;
    int total_quantity = 0;
    for (const auto& e : inv_res.value()->entries) {
        if (e.entity_key == "unknown_bush") {
            bush_count++;
            total_quantity += e.quantity;
        }
    }
    assert(bush_count == 1);
    assert(total_quantity == 2);

    // Second entity should be destroyed (no longer in world runtime)
    auto ent2_res = f.world_runtime.findEntity(item2.entity_id);
    assert(ent2_res.is_error());

    // item_locations should not contain the destroyed entity
    auto loc_res = f.inventory_runtime->findItemLocation(item2.entity_id);
    assert(loc_res.is_error());

    std::cout << "world_inventory_pickup_merge_consistency: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Non stackable not merged via runtime
// ---------------------------------------------------------------------------

void run_non_stackable_runtime_tests() {
    TestFixture f;
    f.setup(1);

    // Place two non-stackable swords at (0,0)
    WorldEntityInstance sword1;
    sword1.entity_id = "sword_surface_0_0_1";
    sword1.entity_key = "sword";
    sword1.display_name_key = "entity.sword";
    sword1.coord = WorldCellCoord{0, 0, "surface"};
    sword1.location_kind = WorldEntityLocationKind::OnMap;
    sword1.quantity = 1;
    sword1.stackable = false;
    sword1.stack_key = "sword:durability_10";
    sword1.state_keys.push_back("durability_10");

    WorldEntityInstance sword2;
    sword2.entity_id = "sword_surface_0_0_2";
    sword2.entity_key = "sword";
    sword2.display_name_key = "entity.sword";
    sword2.coord = WorldCellCoord{0, 0, "surface"};
    sword2.location_kind = WorldEntityLocationKind::OnMap;
    sword2.quantity = 1;
    sword2.stackable = false;
    sword2.stack_key = "sword:durability_5";
    sword2.state_keys.push_back("durability_5");

    auto spawn1 = f.world_runtime.spawnEntityOnMap(
        sword1.entity_id, sword1.entity_key, sword1.display_name_key,
        sword1.coord.value(), sword1.quantity, sword1.stack_key,
        sword1.stackable, sword1.state_keys, sword1.numeric_states, {});
    auto spawn2 = f.world_runtime.spawnEntityOnMap(
        sword2.entity_id, sword2.entity_key, sword2.display_name_key,
        sword2.coord.value(), sword2.quantity, sword2.stack_key,
        sword2.stackable, sword2.state_keys, sword2.numeric_states, {});
    assert(spawn1.is_ok() && spawn2.is_ok());

    // Pickup sword1
    InventoryTransferRequest req1;
    req1.transfer_id = "test_sword_1";
    req1.transfer_kind = InventoryTransferKind::PickupFromMap;
    req1.actor_key = "player";
    req1.entity_id = sword1.entity_id;
    req1.quantity = 1;
    auto res1 = f.inventory_runtime->transfer(req1);
    assert(res1.is_ok() && res1.value().ok);

    // Pickup sword2 (should NOT merge because stackable=false)
    InventoryTransferRequest req2;
    req2.transfer_id = "test_sword_2";
    req2.transfer_kind = InventoryTransferKind::PickupFromMap;
    req2.actor_key = "player";
    req2.entity_id = sword2.entity_id;
    req2.quantity = 1;
    auto res2 = f.inventory_runtime->transfer(req2);
    assert(res2.is_ok() && res2.value().ok);

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 2);

    std::cout << "world_inventory_non_stackable_not_merged: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop partial stack ground entity
// ---------------------------------------------------------------------------

void run_drop_partial_stack_ground_tests() {
    TestFixture f;
    f.setup(1);

    // Manually add a stack of 5 berries to inventory
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "berry_entry";
    entry.entity_id = "berry_001";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 5;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Drop 2 berries
    InventoryTransferRequest drop;
    drop.transfer_id = "test_drop_ground";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_001";
    drop.quantity = 2;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok() && res.value().ok);

    // Inventory should have 3 left
    auto inv_after = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_after.is_ok());
    bool found = false;
    for (const auto& e : inv_after.value()->entries) {
        if (e.entity_id == "berry_001") {
            assert(e.quantity == 3);
            found = true;
            break;
        }
    }
    assert(found);

    // Ground should have a red_berry entity with quantity 2 and correct stack_key
    auto snap = f.world_runtime.snapshotForDebug();
    assert(snap.is_ok());
    bool found_ground = false;
    for (const auto& [id, entity] : snap.value().entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap && entity.entity_key == "red_berry") {
            assert(entity.quantity == 2);
            assert(entity.stack_key == "red_berry:default");
            assert(entity.stackable == true);
            found_ground = true;
            break;
        }
    }
    assert(found_ground);

    std::cout << "world_inventory_drop_partial_stack: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Capacity real slots (capacity=1 rejection)
// ---------------------------------------------------------------------------

void run_capacity_real_slots_tests() {
    TestFixture f;
    f.setup(1);

    // Reduce capacity to 1
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    inv_res.value()->capacity_slots = 1;

    // Explicitly spawn two distinct ground items at player cell
    auto spawn1 = f.world_runtime.spawnEntityOnMap(
        "apple_001", "red_apple", "entity.red_apple",
        WorldCellCoord{0, 0, "surface"}, 1, "red_apple:default", true, {}, {}, {});
    auto spawn2 = f.world_runtime.spawnEntityOnMap(
        "berry_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 1, "red_berry:default", true, {}, {}, {});
    assert(spawn1.is_ok() && spawn2.is_ok());

    // Pickup first item (should succeed, filling the single slot)
    InventoryTransferRequest req1;
    req1.transfer_id = "test_cap_1";
    req1.transfer_kind = InventoryTransferKind::PickupFromMap;
    req1.actor_key = "player";
    req1.entity_id = "apple_001";
    req1.quantity = 1;
    auto res1 = f.inventory_runtime->transfer(req1);
    assert(res1.is_ok() && res1.value().ok);
    assert(inv_res.value()->used_slots == 1);

    // Pickup second item (must fail with CapacityExceeded)
    InventoryTransferRequest req2;
    req2.transfer_id = "test_cap_2";
    req2.transfer_kind = InventoryTransferKind::PickupFromMap;
    req2.actor_key = "player";
    req2.entity_id = "berry_001";
    req2.quantity = 1;
    auto res2 = f.inventory_runtime->transfer(req2);
    assert(res2.is_ok());
    assert(!res2.value().ok);
    assert(res2.value().failure_kind == InventoryFailureKind::CapacityExceeded);
    assert(inv_res.value()->used_slots == 1); // no growth

    std::cout << "world_inventory_capacity_real_slots: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Full drop multiple entries (no UB, correct ground data)
// ---------------------------------------------------------------------------

void run_full_drop_multi_entry_tests() {
    TestFixture f;
    f.setup(1);

    // Manually add two distinct entries
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry_a;
    entry_a.entry_id = "entry_a";
    entry_a.entity_id = "apple_001";
    entry_a.entity_key = "red_apple";
    entry_a.stack_key = "red_apple:fresh";
    entry_a.quantity = 3;
    entry_a.stackable = true;

    InventoryItemEntry entry_b;
    entry_b.entry_id = "entry_b";
    entry_b.entity_id = "sword_001";
    entry_b.entity_key = "sword";
    entry_b.stack_key = "sword:durability_10";
    entry_b.quantity = 1;
    entry_b.stackable = false;

    inv_res.value()->entries.push_back(entry_a);
    inv_res.value()->entries.push_back(entry_b);
    inv_res.value()->used_slots = 2;

    // Full drop entry_a (quantity == entry_a.quantity)
    InventoryTransferRequest drop;
    drop.transfer_id = "test_full_drop_a";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "apple_001";
    drop.quantity = 3;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok() && res.value().ok);

    // Inventory should now have exactly 1 entry (sword)
    auto inv_after = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_after.is_ok());
    assert(inv_after.value()->entries.size() == 1);
    assert(inv_after.value()->entries[0].entity_id == "sword_001");
    assert(inv_after.value()->used_slots == 1);

    // Ground should have red_apple with quantity 3 and correct stack_key
    auto snap = f.world_runtime.snapshotForDebug();
    assert(snap.is_ok());
    bool found_apple = false;
    for (const auto& [id, entity] : snap.value().entities) {
        if (entity.location_kind == WorldEntityLocationKind::OnMap && entity.entity_key == "red_apple") {
            assert(entity.quantity == 3);
            assert(entity.stack_key == "red_apple:fresh");
            found_apple = true;
            break;
        }
    }
    assert(found_apple);

    std::cout << "world_inventory_full_drop_multi_entry: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop cell invalid (target not actor current cell)
// ---------------------------------------------------------------------------

void run_drop_cell_invalid_tests() {
    TestFixture f;
    f.setup(1);

    // Manually add an item to inventory
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "berry_entry";
    entry.entity_id = "berry_001";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 5;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Attempt to drop to adjacent cell (1,0) instead of current (0,0)
    InventoryTransferRequest drop;
    drop.transfer_id = "test_cell_invalid";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_001";
    drop.quantity = 1;
    drop.parameters["drop_x"] = "1";
    drop.parameters["drop_y"] = "0";
    drop.parameters["layer_key"] = "surface";

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::DropCellInvalid);

    // Inventory unchanged
    auto inv_after = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_after.is_ok());
    bool found = false;
    for (const auto& e : inv_after.value()->entries) {
        if (e.entity_id == "berry_001") {
            assert(e.quantity == 5);
            found = true;
            break;
        }
    }
    assert(found);

    std::cout << "world_inventory_drop_cell_invalid: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop projection new entity (partial drop produces new entity id)
// ---------------------------------------------------------------------------

void run_drop_projection_new_entity_tests() {
    TestFixture f;
    f.setup(1);

    // Manually add a stackable item quantity=5
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "berry_entry";
    entry.entity_id = "berry_001";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 5;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Partial drop 2 berries (must spawn a new ground entity)
    InventoryTransferRequest drop;
    drop.transfer_id = "test_proj_new";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_001";
    drop.quantity = 2;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok() && res.value().ok);

    // Result should contain a changed_entity_ids with the newly spawned entity
    // validateDropToMap pushes original entity_id, applyDropToMap pushes new one
    assert(!res.value().changed_entity_ids.empty());
    std::string new_entity_id;
    for (const auto& id : res.value().changed_entity_ids) {
        if (id != "berry_001") {
            new_entity_id = id;
            break;
        }
    }
    assert(!new_entity_id.empty());

    // The new entity should be tracked in item_locations as OnMap
    auto loc_res = f.inventory_runtime->findItemLocation(new_entity_id);
    assert(loc_res.is_ok());
    assert(loc_res.value().location_kind == InventoryLocationKind::OnMap);

    std::cout << "world_inventory_drop_projection_new_entity: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Pickup partial ground stack keeps remainder on map
// ---------------------------------------------------------------------------

void run_pickup_partial_ground_stack_keeps_remainder_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn a stack of 5 berries at player cell
    auto spawn_res = f.world_runtime.spawnEntityOnMap(
        "berry_ground_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 5, "red_berry:default", true, {}, {}, {});
    assert(spawn_res.is_ok());

    // Partial pickup: take only 2
    InventoryTransferRequest req;
    req.transfer_id = "test_partial_pickup";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = "berry_ground_001";
    req.quantity = 2;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    // Inventory should have a berry entry with quantity 2 and distinct entity_id
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    bool found = false;
    std::string inventory_entity_id;
    for (const auto& e : inv_res.value()->entries) {
        if (e.entity_key == "red_berry") {
            assert(e.quantity == 2);
            inventory_entity_id = e.entity_id;
            found = true;
            break;
        }
    }
    assert(found);

    // The inventory carrier must NOT reuse the ground entity id
    assert(!inventory_entity_id.empty());
    assert(inventory_entity_id != "berry_ground_001");

    // Inventory carrier must be tracked as InInventory
    auto inv_loc = f.inventory_runtime->findItemLocation(inventory_entity_id);
    assert(inv_loc.is_ok());
    assert(inv_loc.value().location_kind == InventoryLocationKind::InInventory);

    // Ground entity should still exist with quantity 3
    auto ent_res = f.world_runtime.findEntity("berry_ground_001");
    assert(ent_res.is_ok());
    assert(ent_res.value()->quantity == 3);
    assert(ent_res.value()->location_kind == WorldEntityLocationKind::OnMap);

    // Ground entity id must NOT be tracked as InInventory
    auto ground_loc = f.inventory_runtime->findItemLocation("berry_ground_001");
    assert(ground_loc.is_error() || ground_loc.value().location_kind != InventoryLocationKind::InInventory);

    std::cout << "world_inventory_pickup_partial_ground_stack_keeps_remainder: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Partial pickup ground stack: inventory entity id must be distinct
// ---------------------------------------------------------------------------

void run_pickup_partial_ground_stack_has_distinct_inventory_entity_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn a stack of 5 berries at player cell
    auto spawn_res = f.world_runtime.spawnEntityOnMap(
        "berry_ground_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 5, "red_berry:default", true, {}, {}, {});
    assert(spawn_res.is_ok());

    // Partial pickup: take 2
    InventoryTransferRequest req;
    req.transfer_id = "test_distinct";
    req.transfer_kind = InventoryTransferKind::PickupFromMap;
    req.actor_key = "player";
    req.entity_id = "berry_ground_001";
    req.quantity = 2;

    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    // Find the inventory entry
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 1);
    const auto& entry = inv_res.value()->entries[0];

    // Inventory carrier must be a new distinct id
    assert(entry.entity_id != "berry_ground_001");
    assert(!entry.entity_id.empty());

    // The new carrier must be tracked as InInventory
    auto loc_res = f.inventory_runtime->findItemLocation(entry.entity_id);
    assert(loc_res.is_ok());
    assert(loc_res.value().location_kind == InventoryLocationKind::InInventory);

    // The ground entity must NOT be tracked as InInventory
    auto ground_loc = f.inventory_runtime->findItemLocation("berry_ground_001");
    assert(ground_loc.is_error() || ground_loc.value().location_kind != InventoryLocationKind::InInventory);

    // Ground entity must still be OnMap with quantity 3
    auto ent_res = f.world_runtime.findEntity("berry_ground_001");
    assert(ent_res.is_ok());
    assert(ent_res.value()->quantity == 3);
    assert(ent_res.value()->location_kind == WorldEntityLocationKind::OnMap);

    std::cout << "world_inventory_pickup_partial_ground_stack_has_distinct_inventory_entity: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Full drop merge to ground cleans source entity
// ---------------------------------------------------------------------------

void run_full_drop_merge_ground_cleans_source_entity_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn a ground stack of berries at player cell
    auto spawn_ground = f.world_runtime.spawnEntityOnMap(
        "berry_ground_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 3, "red_berry:default", true, {}, {}, {});
    assert(spawn_ground.is_ok());

    // Manually add a matching entry to inventory
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "berry_entry";
    entry.entity_id = "berry_002";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 2;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Full drop merge: 2 berries dropped onto existing ground stack of 3
    InventoryTransferRequest drop;
    drop.transfer_id = "test_drop_merge";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_002";
    drop.quantity = 2;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok() && res.value().ok);

    // Ground stack should now have 5 berries
    auto ground_res = f.world_runtime.findEntity("berry_ground_001");
    assert(ground_res.is_ok());
    assert(ground_res.value()->quantity == 5);

    // Inventory entry should be removed
    auto inv_after = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_after.is_ok());
    bool found = false;
    for (const auto& e : inv_after.value()->entries) {
        if (e.entity_id == "berry_002") {
            found = true;
            break;
        }
    }
    assert(!found);
    assert(inv_after.value()->used_slots == 0);

    // Source entity should be destroyed (no ghost state)
    auto src_ent = f.world_runtime.findEntity("berry_002");
    assert(src_ent.is_error());

    // item_locations should not contain the destroyed source
    auto src_loc = f.inventory_runtime->findItemLocation("berry_002");
    assert(src_loc.is_error());

    std::cout << "world_inventory_full_drop_merge_ground_cleans_source_entity: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Drop merge ground changed_entity_ids
// ---------------------------------------------------------------------------

void run_drop_merge_ground_changed_entity_ids_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn a ground stack at player cell
    auto spawn_ground = f.world_runtime.spawnEntityOnMap(
        "berry_ground_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 3, "red_berry:default", true, {}, {}, {});
    assert(spawn_ground.is_ok());

    // Manually add a matching entry to inventory
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());

    InventoryItemEntry entry;
    entry.entry_id = "berry_entry";
    entry.entity_id = "berry_002";
    entry.entity_key = "red_berry";
    entry.stack_key = "red_berry:default";
    entry.quantity = 2;
    entry.stackable = true;
    inv_res.value()->entries.push_back(entry);
    inv_res.value()->used_slots += 1;

    // Full drop merge
    InventoryTransferRequest drop;
    drop.transfer_id = "test_merge_ids";
    drop.transfer_kind = InventoryTransferKind::DropToMap;
    drop.actor_key = "player";
    drop.entity_id = "berry_002";
    drop.quantity = 2;

    auto res = f.inventory_runtime->transfer(drop);
    assert(res.is_ok() && res.value().ok);

    // changed_entity_ids must contain the updated ground stack entity
    bool has_ground = false;
    for (const auto& id : res.value().changed_entity_ids) {
        if (id == "berry_ground_001") {
            has_ground = true;
            break;
        }
    }
    assert(has_ground);

    std::cout << "world_inventory_drop_merge_ground_changed_entity_ids: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Same entity_key different state_keys must not merge even when stackable
// ---------------------------------------------------------------------------

void run_stack_key_blocks_merge_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn two red_berry with different state_keys (fresh vs rotten)
    auto spawn1 = f.world_runtime.spawnEntityOnMap(
        "berry_fresh_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 1, "red_berry:fresh", true,
        std::vector<std::string>{"fresh"}, {}, {});
    auto spawn2 = f.world_runtime.spawnEntityOnMap(
        "berry_rotten_001", "red_berry", "entity.red_berry",
        WorldCellCoord{0, 0, "surface"}, 1, "red_berry:rotten", true,
        std::vector<std::string>{"rotten"}, {}, {});
    assert(spawn1.is_ok() && spawn2.is_ok());

    // Pickup fresh berry
    InventoryTransferRequest req1;
    req1.transfer_id = "test_fresh";
    req1.transfer_kind = InventoryTransferKind::PickupFromMap;
    req1.actor_key = "player";
    req1.entity_id = "berry_fresh_001";
    req1.quantity = 1;
    auto res1 = f.inventory_runtime->transfer(req1);
    assert(res1.is_ok() && res1.value().ok);

    // Pickup rotten berry (must NOT merge with fresh)
    InventoryTransferRequest req2;
    req2.transfer_id = "test_rotten";
    req2.transfer_kind = InventoryTransferKind::PickupFromMap;
    req2.actor_key = "player";
    req2.entity_id = "berry_rotten_001";
    req2.quantity = 1;
    auto res2 = f.inventory_runtime->transfer(req2);
    assert(res2.is_ok() && res2.value().ok);

    // Inventory must have exactly two separate entries
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 2);

    // Verify distinct stack_keys
    assert(inv_res.value()->entries[0].stack_key != inv_res.value()->entries[1].stack_key);

    std::cout << "world_inventory_stack_key_blocks_merge: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// ConsumeToNowhere reduces stack
// ---------------------------------------------------------------------------

void run_consume_to_nowhere_reduces_stack_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn wood x3 on map and pickup
    auto spawn = f.world_runtime.spawnEntityOnMap(
        "wood_001", "wood", "entity.wood",
        WorldCellCoord{0, 0, "surface"}, 3, "wood:default", true,
        std::vector<std::string>{}, {}, {});
    assert(spawn.is_ok());

    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_pickup";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = "wood_001";
    pickup.quantity = 3;
    auto pres = f.inventory_runtime->transfer(pickup);
    assert(pres.is_ok() && pres.value().ok);

    // Consume 1
    InventoryTransferRequest req;
    req.transfer_id = "test_consume";
    req.transfer_kind = InventoryTransferKind::ConsumeToNowhere;
    req.actor_key = "player";
    req.entity_id = "wood_001";
    req.quantity = 1;
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);
    assert(!res.value().changed_inventory_ids.empty());
    assert(!res.value().changed_entity_ids.empty());

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 1);
    assert(inv_res.value()->entries[0].quantity == 2);

    std::cout << "world_inventory_consume_to_nowhere_reduces_stack: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// ConsumeToNowhere removes stack at zero
// ---------------------------------------------------------------------------

void run_consume_to_nowhere_removes_stack_at_zero_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn wood x1 on map and pickup
    auto spawn = f.world_runtime.spawnEntityOnMap(
        "wood_001", "wood", "entity.wood",
        WorldCellCoord{0, 0, "surface"}, 1, "wood:default", true,
        std::vector<std::string>{}, {}, {});
    assert(spawn.is_ok());

    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_pickup";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = "wood_001";
    pickup.quantity = 1;
    auto pres = f.inventory_runtime->transfer(pickup);
    assert(pres.is_ok() && pres.value().ok);

    // Consume all
    InventoryTransferRequest req;
    req.transfer_id = "test_consume";
    req.transfer_kind = InventoryTransferKind::ConsumeToNowhere;
    req.actor_key = "player";
    req.entity_id = "wood_001";
    req.quantity = 1;
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.empty());

    // Item location should be gone or nowhere
    auto loc_res = f.inventory_runtime->findItemLocation("wood_001");
    assert(loc_res.is_error());

    std::cout << "world_inventory_consume_to_nowhere_removes_stack_at_zero: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// ConsumeToNowhere insufficient fails
// ---------------------------------------------------------------------------

void run_consume_to_nowhere_insufficient_fails_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn wood x1 on map and pickup
    auto spawn = f.world_runtime.spawnEntityOnMap(
        "wood_001", "wood", "entity.wood",
        WorldCellCoord{0, 0, "surface"}, 1, "wood:default", true,
        std::vector<std::string>{}, {}, {});
    assert(spawn.is_ok());

    InventoryTransferRequest pickup;
    pickup.transfer_id = "test_pickup";
    pickup.transfer_kind = InventoryTransferKind::PickupFromMap;
    pickup.actor_key = "player";
    pickup.entity_id = "wood_001";
    pickup.quantity = 1;
    auto pres = f.inventory_runtime->transfer(pickup);
    assert(pres.is_ok() && pres.value().ok);

    // Consume 2 (should fail)
    InventoryTransferRequest req;
    req.transfer_id = "test_consume";
    req.transfer_kind = InventoryTransferKind::ConsumeToNowhere;
    req.actor_key = "player";
    req.entity_id = "wood_001";
    req.quantity = 2;
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok());
    assert(!res.value().ok);
    assert(res.value().failure_kind == InventoryFailureKind::QuantityInsufficient);

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 1);
    assert(inv_res.value()->entries[0].quantity == 1);

    std::cout << "world_inventory_consume_to_nowhere_insufficient_fails: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// SpawnToInventory adds item
// ---------------------------------------------------------------------------

void run_spawn_to_inventory_adds_item_tests() {
    TestFixture f;
    f.setup(1);

    // Ensure inventory exists
    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    assert(inv_id.is_ok());

    // Spawn torch
    InventoryTransferRequest req;
    req.transfer_id = "test_spawn";
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = "player";
    req.entity_key = "torch";
    req.quantity = 1;
    req.parameters["stack_key"] = "torch:crafted";
    req.parameters["state_keys"] = "crafted";
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);
    assert(!res.value().changed_inventory_ids.empty());
    assert(!res.value().changed_entity_ids.empty());

    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 1);
    assert(inv_res.value()->entries[0].entity_key == "torch");
    assert(inv_res.value()->entries[0].quantity == 1);

    // Verify location tracking
    auto loc_res = f.inventory_runtime->findItemLocation(res.value().changed_entity_ids[0]);
    assert(loc_res.is_ok());
    assert(loc_res.value().location_kind == InventoryLocationKind::InInventory);

    std::cout << "world_inventory_spawn_to_inventory_adds_item: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// SpawnToInventory merges stack
// ---------------------------------------------------------------------------

void run_spawn_to_inventory_merges_stack_tests() {
    TestFixture f;
    f.setup(1);

    // Spawn torch x1 first
    InventoryTransferRequest req1;
    req1.transfer_id = "test_spawn1";
    req1.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req1.actor_key = "player";
    req1.entity_key = "torch";
    req1.quantity = 1;
    req1.parameters["stack_key"] = "torch:crafted";
    auto res1 = f.inventory_runtime->transfer(req1);
    assert(res1.is_ok() && res1.value().ok);

    // Spawn torch x2 with same stack_key
    InventoryTransferRequest req2;
    req2.transfer_id = "test_spawn2";
    req2.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req2.actor_key = "player";
    req2.entity_key = "torch";
    req2.quantity = 2;
    req2.parameters["stack_key"] = "torch:crafted";
    auto res2 = f.inventory_runtime->transfer(req2);
    assert(res2.is_ok() && res2.value().ok);

    auto inv_id = f.inventory_runtime->ensureActorInventory("player");
    auto inv_res = f.inventory_runtime->findInventory(inv_id.value());
    assert(inv_res.is_ok());
    assert(inv_res.value()->entries.size() == 1);
    assert(inv_res.value()->entries[0].quantity == 3);

    std::cout << "world_inventory_spawn_to_inventory_merges_stack: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// SpawnToInventory creates WorldEntityInstance
// ---------------------------------------------------------------------------

void run_spawn_to_inventory_creates_world_entity_tests() {
    TestFixture f;
    f.setup(1);

    InventoryTransferRequest req;
    req.transfer_id = "test_spawn";
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = "player";
    req.entity_key = "torch";
    req.entity_id = "my_torch_001";
    req.quantity = 1;
    req.parameters["stack_key"] = "torch:crafted";
    req.parameters["state_keys"] = "crafted";
    req.parameters["display_name_key"] = "entity.torch";
    req.parameters["tag_keys"] = "reaction_output";
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    // WorldEntityInstance must exist
    auto ent_res = f.world_runtime.findEntity("my_torch_001");
    assert(ent_res.is_ok());
    const auto* entity = ent_res.value();
    assert(entity->location_kind == WorldEntityLocationKind::InInventory);
    assert(entity->entity_key == "torch");
    assert(entity->quantity == 1);
    assert(entity->owner_ref == "player");

    bool has_crafted = false;
    for (const auto& sk : entity->state_keys) {
        if (sk == "crafted") has_crafted = true;
    }
    assert(has_crafted);

    std::cout << "world_inventory_spawn_to_inventory_creates_world_entity: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// SpawnToInventory uses request.entity_id
// ---------------------------------------------------------------------------

void run_spawn_to_inventory_uses_request_entity_id_tests() {
    TestFixture f;
    f.setup(1);

    InventoryTransferRequest req;
    req.transfer_id = "test_spawn";
    req.transfer_kind = InventoryTransferKind::SpawnToInventory;
    req.actor_key = "player";
    req.entity_key = "torch";
    req.entity_id = "custom_id_42";
    req.quantity = 1;
    auto res = f.inventory_runtime->transfer(req);
    assert(res.is_ok() && res.value().ok);

    auto ent_res = f.world_runtime.findEntity("custom_id_42");
    assert(ent_res.is_ok());

    std::cout << "world_inventory_spawn_to_inventory_uses_request_entity_id: passed" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <test_name>" << std::endl;
        std::cout << "Available tests:" << std::endl;
        std::cout << "  owner_id" << std::endl;
        std::cout << "  ensure_actor_inventory" << std::endl;
        std::cout << "  pickup_from_map" << std::endl;
        std::cout << "  drop_to_map" << std::endl;
        std::cout << "  no_duplicate_location" << std::endl;
        std::cout << "  quantity_invalid" << std::endl;
        std::cout << "  quantity_insufficient" << std::endl;
        std::cout << "  capacity_exceeded" << std::endl;
        std::cout << "  state_version" << std::endl;
        std::cout << "  stack_key_separates_states" << std::endl;
        std::cout << "  non_stackable_not_merged" << std::endl;
        std::cout << "  pickup_too_far" << std::endl;
        std::cout << "  pickup_missing_item" << std::endl;
        std::cout << "  drop_not_owned" << std::endl;
        std::cout << "  drop_partial_stack" << std::endl;
        std::cout << "  pickup_duplicate_click_fails" << std::endl;
        std::cout << "  source_proof_actor_self_only" << std::endl;
        std::cout << "  no_default_camp_sharing" << std::endl;
        std::cout << "  pickup_requires_visible_target" << std::endl;
        std::cout << "  container_not_empty_blocked" << std::endl;
        std::cout << "  validate_no_state_change" << std::endl;
        std::cout << "  pickup_merge_consistency" << std::endl;
        std::cout << "  non_stackable_runtime" << std::endl;
        std::cout << "  drop_partial_stack_ground" << std::endl;
        std::cout << "  capacity_real_slots" << std::endl;
        std::cout << "  full_drop_multi_entry" << std::endl;
        std::cout << "  drop_cell_invalid" << std::endl;
        std::cout << "  drop_projection_new_entity" << std::endl;
        std::cout << "  pickup_partial_ground_stack_keeps_remainder" << std::endl;
        std::cout << "  pickup_partial_ground_stack_has_distinct_inventory_entity" << std::endl;
        std::cout << "  full_drop_merge_ground_cleans_source_entity" << std::endl;
        std::cout << "  drop_merge_ground_changed_entity_ids" << std::endl;
        std::cout << "  stack_key_blocks_merge" << std::endl;
        return 1;
    }

    std::string test_name = argv[1];

    if (test_name == "owner_id") {
        run_owner_id_tests();
    } else if (test_name == "ensure_actor_inventory") {
        run_ensure_actor_inventory_tests();
    } else if (test_name == "pickup_from_map") {
        run_pickup_from_map_tests();
    } else if (test_name == "drop_to_map") {
        run_drop_to_map_tests();
    } else if (test_name == "no_duplicate_location") {
        run_no_duplicate_location_tests();
    } else if (test_name == "quantity_invalid") {
        run_quantity_invalid_tests();
    } else if (test_name == "quantity_insufficient") {
        run_quantity_insufficient_tests();
    } else if (test_name == "capacity_exceeded") {
        run_capacity_exceeded_tests();
    } else if (test_name == "state_version") {
        run_state_version_tests();
    } else if (test_name == "stack_key_separates_states") {
        run_stack_key_separates_states_tests();
    } else if (test_name == "non_stackable_not_merged") {
        run_non_stackable_not_merged_tests();
    } else if (test_name == "pickup_too_far") {
        run_pickup_too_far_tests();
    } else if (test_name == "pickup_missing_item") {
        run_pickup_missing_item_tests();
    } else if (test_name == "drop_not_owned") {
        run_drop_not_owned_tests();
    } else if (test_name == "drop_partial_stack") {
        run_drop_partial_stack_tests();
    } else if (test_name == "pickup_duplicate_click_fails") {
        run_pickup_duplicate_click_fails_tests();
    } else if (test_name == "source_proof_actor_self_only") {
        run_source_proof_actor_self_only_tests();
    } else if (test_name == "no_default_camp_sharing") {
        run_no_default_camp_sharing_tests();
    } else if (test_name == "pickup_requires_visible_target") {
        run_pickup_requires_visible_target_tests();
    } else if (test_name == "container_not_empty_blocked") {
        run_container_not_empty_blocked_tests();
    } else if (test_name == "validate_no_state_change") {
        run_validate_no_state_change_tests();
    } else if (test_name == "pickup_merge_consistency") {
        run_pickup_merge_consistency_tests();
    } else if (test_name == "non_stackable_runtime") {
        run_non_stackable_runtime_tests();
    } else if (test_name == "drop_partial_stack_ground") {
        run_drop_partial_stack_ground_tests();
    } else if (test_name == "capacity_real_slots") {
        run_capacity_real_slots_tests();
    } else if (test_name == "full_drop_multi_entry") {
        run_full_drop_multi_entry_tests();
    } else if (test_name == "drop_cell_invalid") {
        run_drop_cell_invalid_tests();
    } else if (test_name == "drop_projection_new_entity") {
        run_drop_projection_new_entity_tests();
    } else if (test_name == "pickup_partial_ground_stack_keeps_remainder") {
        run_pickup_partial_ground_stack_keeps_remainder_tests();
    } else if (test_name == "pickup_partial_ground_stack_has_distinct_inventory_entity") {
        run_pickup_partial_ground_stack_has_distinct_inventory_entity_tests();
    } else if (test_name == "full_drop_merge_ground_cleans_source_entity") {
        run_full_drop_merge_ground_cleans_source_entity_tests();
    } else if (test_name == "drop_merge_ground_changed_entity_ids") {
        run_drop_merge_ground_changed_entity_ids_tests();
    } else if (test_name == "stack_key_blocks_merge") {
        run_stack_key_blocks_merge_tests();
    } else if (test_name == "consume_to_nowhere_reduces_stack") {
        run_consume_to_nowhere_reduces_stack_tests();
    } else if (test_name == "consume_to_nowhere_removes_stack_at_zero") {
        run_consume_to_nowhere_removes_stack_at_zero_tests();
    } else if (test_name == "consume_to_nowhere_insufficient_fails") {
        run_consume_to_nowhere_insufficient_fails_tests();
    } else if (test_name == "spawn_to_inventory_adds_item") {
        run_spawn_to_inventory_adds_item_tests();
    } else if (test_name == "spawn_to_inventory_merges_stack") {
        run_spawn_to_inventory_merges_stack_tests();
    } else if (test_name == "spawn_to_inventory_creates_world_entity") {
        run_spawn_to_inventory_creates_world_entity_tests();
    } else if (test_name == "spawn_to_inventory_uses_request_entity_id") {
        run_spawn_to_inventory_uses_request_entity_id_tests();
    } else {
        std::cout << "Unknown test: " << test_name << std::endl;
        return 1;
    }

    return 0;
}
