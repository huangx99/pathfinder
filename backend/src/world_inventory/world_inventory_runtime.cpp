#include "pathfinder/world_inventory/world_inventory_runtime.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::world_inventory {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorDetail;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_command;

// ---------------------------------------------------------------------------
// Construction / Initialization
// ---------------------------------------------------------------------------

WorldInventoryRuntime::WorldInventoryRuntime(IWorldEntityLocationPort& world_port)
    : world_port_(world_port) {}

Result<void> WorldInventoryRuntime::initialize() {
    state_version_ = 1;
    inventories_.clear();
    item_locations_.clear();
    return Result<void>::ok();
}

uint64_t WorldInventoryRuntime::stateVersion() const {
    return state_version_;
}

void WorldInventoryRuntime::incrementStateVersion() {
    ++state_version_;
}

// ---------------------------------------------------------------------------
// Inventory ID helpers
// ---------------------------------------------------------------------------

std::string WorldInventoryRuntime::makeInventoryId(const InventoryOwnerRef& owner) const {
    return "inv_" + toString(owner.owner_kind) + "_" + owner.owner_key;
}

std::string WorldInventoryRuntime::makeStackKey(
    const std::string& entity_key,
    const std::vector<std::string>& state_keys,
    const std::map<std::string, double>& numeric_states) const {
    std::string key = entity_key;
    for (const auto& sk : state_keys) {
        key += ":" + sk;
    }
    for (const auto& [k, v] : numeric_states) {
        key += ":" + k + "=" + std::to_string(v);
    }
    if (state_keys.empty() && numeric_states.empty()) {
        key += ":default";
    }
    return key;
}

// ---------------------------------------------------------------------------
// Inventory queries
// ---------------------------------------------------------------------------

Result<InventoryRuntimeState*> WorldInventoryRuntime::findInventory(const std::string& inventory_id) {
    auto it = inventories_.find(inventory_id);
    if (it == inventories_.end()) {
        return Result<InventoryRuntimeState*>::fail(
            makeError(ErrorCode::id_not_found, "inventory_not_found", "Inventory not found: " + inventory_id));
    }
    return Result<InventoryRuntimeState*>::ok(&it->second);
}

Result<const InventoryRuntimeState*> WorldInventoryRuntime::findInventory(const std::string& inventory_id) const {
    auto it = inventories_.find(inventory_id);
    if (it == inventories_.end()) {
        return Result<const InventoryRuntimeState*>::fail(
            makeError(ErrorCode::id_not_found, "inventory_not_found", "Inventory not found: " + inventory_id));
    }
    return Result<const InventoryRuntimeState*>::ok(&it->second);
}

Result<std::string> WorldInventoryRuntime::ensureActorInventory(const std::string& actor_key) {
    InventoryOwnerRef owner{InventoryOwnerKind::Actor, actor_key};
    std::string inventory_id = makeInventoryId(owner);
    auto it = inventories_.find(inventory_id);
    if (it != inventories_.end()) {
        return Result<std::string>::ok(inventory_id);
    }
    InventoryRuntimeState state;
    state.inventory_id = inventory_id;
    state.owner = owner;
    state.capacity_slots = 20;
    state.used_slots = 0;
    state.public_read = false;
    state.public_take = false;
    inventories_[inventory_id] = std::move(state);
    incrementStateVersion();
    return Result<std::string>::ok(inventory_id);
}

Result<std::string> WorldInventoryRuntime::ensureContainerInventory(const std::string& container_entity_id) {
    InventoryOwnerRef owner{InventoryOwnerKind::Container, container_entity_id};
    std::string inventory_id = makeInventoryId(owner);
    auto it = inventories_.find(inventory_id);
    if (it != inventories_.end()) {
        return Result<std::string>::ok(inventory_id);
    }
    InventoryRuntimeState state;
    state.inventory_id = inventory_id;
    state.owner = owner;
    state.capacity_slots = 20;
    state.used_slots = 0;
    state.public_read = true;
    state.public_take = false;
    inventories_[inventory_id] = std::move(state);
    incrementStateVersion();
    return Result<std::string>::ok(inventory_id);
}

Result<ItemLocationRef> WorldInventoryRuntime::findItemLocation(const std::string& entity_id) const {
    auto it = item_locations_.find(entity_id);
    if (it == item_locations_.end()) {
        return Result<ItemLocationRef>::fail(
            makeError(ErrorCode::id_not_found, "item_location_not_found", "Item location not found: " + entity_id));
    }
    return Result<ItemLocationRef>::ok(it->second);
}

Result<std::vector<InventoryItemEntry>> WorldInventoryRuntime::queryItems(
    const InventoryOwnerRef& owner, const std::string& entity_key) const {
    std::vector<InventoryItemEntry> result;
    std::string inventory_id = makeInventoryId(owner);
    auto inv_res = findInventory(inventory_id);
    if (inv_res.is_error()) {
        return Result<std::vector<InventoryItemEntry>>::ok(std::move(result));
    }
    for (const auto& entry : inv_res.value()->entries) {
        if (entity_key.empty() || entry.entity_key == entity_key) {
            result.push_back(entry);
        }
    }
    return Result<std::vector<InventoryItemEntry>>::ok(std::move(result));
}

// ---------------------------------------------------------------------------
// findOrCreateActorInventory (intended for apply phase only)
// ---------------------------------------------------------------------------

Result<std::string> WorldInventoryRuntime::findOrCreateActorInventory(const std::string& actor_key) {
    return ensureActorInventory(actor_key);
}

// ---------------------------------------------------------------------------
// isContainerEntity
// ---------------------------------------------------------------------------

bool WorldInventoryRuntime::isContainerEntity(const WorldEntityInstance& entity) const {
    for (const auto& tag : entity.tag_keys) {
        if (tag == "container") return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Adjacency helpers
// ---------------------------------------------------------------------------

bool WorldInventoryRuntime::isAdjacent(const WorldCellCoord& a, const WorldCellCoord& b) const {
    if (a.layer_key != b.layer_key) return false;
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

bool WorldInventoryRuntime::isSameOrAdjacent(const WorldCellCoord& a, const WorldCellCoord& b) const {
    if (a.layer_key != b.layer_key) return false;
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return (dx == 0 && dy == 0) || (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

// ---------------------------------------------------------------------------
// Ground stack merge support
// ---------------------------------------------------------------------------

Result<std::string> WorldInventoryRuntime::findGroundStackEntity(
    const WorldCellCoord& coord,
    const std::string& entity_key,
    const std::string& stack_key) const {
    auto cell_res = world_port_.findCell(coord);
    if (cell_res.is_error()) {
        return Result<std::string>::fail(cell_res.errors()[0]);
    }
    for (const auto& eid : cell_res.value()->entity_ids) {
        auto ent_res = world_port_.findEntity(eid);
        if (ent_res.is_error()) continue;
        const auto* ent = ent_res.value();
        if (ent->entity_key == entity_key && ent->location_kind == WorldEntityLocationKind::OnMap && ent->stack_key == stack_key) {
            return Result<std::string>::ok(eid);
        }
    }
    return Result<std::string>::fail(
        makeError(ErrorCode::id_not_found, "ground_stack_not_found", "No matching ground stack found"));
}

// ---------------------------------------------------------------------------
// Transfer orchestration (draft model)
// ---------------------------------------------------------------------------

Result<InventoryTransferResult> WorldInventoryRuntime::transfer(const InventoryTransferRequest& request) {
    InventoryTransferResult result;
    result.ok = false;
    result.failure_kind = InventoryFailureKind::None;

    // 1. Validate and build draft
    auto draft_res = validateTransfer(request);
    if (draft_res.is_error()) {
        const auto& errors = draft_res.errors();
        if (!errors.empty()) {
            const std::string& mk = errors[0].message_key;
            if (mk == "actor_missing") result.failure_kind = InventoryFailureKind::ActorMissing;
            else if (mk == "item_missing") result.failure_kind = InventoryFailureKind::ItemMissing;
            else if (mk == "item_not_in_inventory") result.failure_kind = InventoryFailureKind::ItemNotInInventory;
            else if (mk == "item_not_in_container") result.failure_kind = InventoryFailureKind::ItemNotInContainer;
            else if (mk == "item_not_on_map") result.failure_kind = InventoryFailureKind::ItemNotOnMap;
            else if (mk == "item_no_coord") result.failure_kind = InventoryFailureKind::ItemNotOnMap;
            else if (mk == "target_cell_missing") result.failure_kind = InventoryFailureKind::TargetCellMissing;
            else if (mk == "target_too_far") result.failure_kind = InventoryFailureKind::TargetTooFar;
            else if (mk == "target_not_visible") result.failure_kind = InventoryFailureKind::TargetNotVisible;
            else if (mk == "quantity_invalid") result.failure_kind = InventoryFailureKind::QuantityInvalid;
            else if (mk == "quantity_insufficient") result.failure_kind = InventoryFailureKind::QuantityInsufficient;
            else if (mk == "capacity_exceeded") result.failure_kind = InventoryFailureKind::CapacityExceeded;
            else if (mk == "forbidden_by_owner") result.failure_kind = InventoryFailureKind::ForbiddenByOwner;
            else if (mk == "container_closed") result.failure_kind = InventoryFailureKind::ContainerClosed;
            else if (mk == "item_not_stackable") result.failure_kind = InventoryFailureKind::ItemNotStackable;
            else if (mk == "conflict_location_state") result.failure_kind = InventoryFailureKind::ConflictLocationState;
            else if (mk == "runtime_mismatch") result.failure_kind = InventoryFailureKind::RuntimeMismatch;
            else if (mk == "container_not_empty") result.failure_kind = InventoryFailureKind::ContainerNotEmpty;
            else if (mk == "item_equipped") result.failure_kind = InventoryFailureKind::ItemEquipped;
            else if (mk == "drop_cell_invalid") result.failure_kind = InventoryFailureKind::DropCellInvalid;
            else result.failure_kind = InventoryFailureKind::RuntimeMismatch;
        } else {
            result.failure_kind = InventoryFailureKind::RuntimeMismatch;
        }
        return Result<InventoryTransferResult>::ok(std::move(result));
    }
    auto draft = std::move(draft_res.value());

    // 2. Apply world map changes
    auto apply_res = applyTransfer(draft);
    if (apply_res.is_error()) {
        result.failure_kind = InventoryFailureKind::ConflictLocationState;
        return Result<InventoryTransferResult>::ok(std::move(result));
    }

    // 3. Build result
    result.ok = true;
    result.changed_inventory_ids = std::move(draft.changed_inventory_ids);
    result.changed_entity_ids = std::move(draft.changed_entity_ids);
    result.changed_cell_ids = std::move(draft.changed_cell_ids);
    incrementStateVersion();

    // Emit event
    WorldEventDto event;
    event.event_id = request.transfer_id + "_transfer";
    event.event_kind = toString(request.transfer_kind);
    event.tick = 0; // caller should set
    event.title_text = "物品转移";
    event.body_text = "Transfer completed: " + toString(request.transfer_kind);
    event.actor_key = request.actor_key;
    result.events.push_back(std::move(event));

    return Result<InventoryTransferResult>::ok(std::move(result));
}

Result<InventoryTransferDraft> WorldInventoryRuntime::validateTransfer(const InventoryTransferRequest& request) {
    switch (request.transfer_kind) {
        case InventoryTransferKind::PickupFromMap:
            return validatePickupFromMap(request);
        case InventoryTransferKind::DropToMap:
            return validateDropToMap(request);
        case InventoryTransferKind::ConsumeToNowhere:
            return validateConsumeToNowhere(request);
        case InventoryTransferKind::SpawnToInventory:
            return validateSpawnToInventory(request);
        default:
            return Result<InventoryTransferDraft>::fail(
                makeError(ErrorCode::command_invalid_argument, "unsupported_transfer_kind",
                          "Unsupported transfer kind: " + toString(request.transfer_kind)));
    }
}

Result<void> WorldInventoryRuntime::applyTransfer(InventoryTransferDraft& draft) {
    switch (draft.request.transfer_kind) {
        case InventoryTransferKind::PickupFromMap:
            return applyPickupFromMap(draft);
        case InventoryTransferKind::DropToMap:
            return applyDropToMap(draft);
        case InventoryTransferKind::ConsumeToNowhere:
            return applyConsumeToNowhere(draft);
        case InventoryTransferKind::SpawnToInventory:
            return applySpawnToInventory(draft);
        default:
            return Result<void>::fail(
                makeError(ErrorCode::command_invalid_argument, "unsupported_transfer_kind",
                          "Unsupported transfer kind: " + toString(draft.request.transfer_kind)));
    }
}

// ---------------------------------------------------------------------------
// PickupFromMap validation
// ---------------------------------------------------------------------------

Result<InventoryTransferDraft> WorldInventoryRuntime::validatePickupFromMap(const InventoryTransferRequest& request) {
    InventoryTransferDraft draft;
    draft.draft_id = request.transfer_id + "_draft";
    draft.request = request;
    draft.validated = false;

    // Validate actor exists
    auto actor_res = world_port_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        draft.request.transfer_kind = InventoryTransferKind::Unknown;
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "actor_missing", "Actor not found: " + request.actor_key));
    }
    const auto* actor = actor_res.value();

    // Validate entity exists
    auto entity_res = world_port_.findEntity(request.entity_id);
    if (entity_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "item_missing", "Item not found: " + request.entity_id));
    }
    const auto* entity = entity_res.value();

    // Validate entity is OnMap
    if (entity->location_kind != WorldEntityLocationKind::OnMap) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::state_change_invalid, "item_not_on_map",
                      "Item is not on map: " + request.entity_id));
    }

    // Validate entity has coord
    if (!entity->coord.has_value()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::state_change_invalid, "item_no_coord",
                      "Item has no coordinate: " + request.entity_id));
    }

    // Validate visibility: target cell must be Visible to actor
    if (!world_port_.isCellVisibleToActor(request.actor_key, entity->coord.value())) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_target_unavailable, "target_not_visible",
                      "Target cell is not visible to actor: " + entity->coord.value().cellId()));
    }

    // Validate distance: same cell or adjacent
    if (!isSameOrAdjacent(actor->coord, entity->coord.value())) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_target_too_far, "target_too_far",
                      "Target is too far from actor"));
    }

    // Validate quantity
    if (request.quantity <= 0 || request.quantity > entity->quantity) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "quantity_invalid",
                      "Invalid quantity: " + std::to_string(request.quantity)));
    }

    // Validate actor inventory capacity (find only, do not create in validate)
    InventoryOwnerRef owner{InventoryOwnerKind::Actor, request.actor_key};
    std::string actor_inventory_id = makeInventoryId(owner);
    int used_slots = 0;
    int capacity_slots = 20;
    auto inv_ptr_res = findInventory(actor_inventory_id);
    if (inv_ptr_res.is_ok()) {
        used_slots = inv_ptr_res.value()->used_slots;
        capacity_slots = inv_ptr_res.value()->capacity_slots;
    }
    if (used_slots >= capacity_slots) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_insufficient_cost, "capacity_exceeded",
                      "Inventory capacity exceeded"));
    }

    // P45: Container not empty check (only if entity has container trait)
    if (isContainerEntity(*entity)) {
        std::string container_inv_id = makeInventoryId(InventoryOwnerRef{InventoryOwnerKind::Container, request.entity_id});
        auto container_inv_res = findInventory(container_inv_id);
        if (container_inv_res.is_ok() && !container_inv_res.value()->entries.empty()) {
            return Result<InventoryTransferDraft>::fail(
                makeError(ErrorCode::state_change_invalid, "container_not_empty",
                          "Cannot pick up non-empty container: " + request.entity_id));
        }
    }

    // Build draft
    draft.source_entity_ids.push_back(request.entity_id);
    draft.source_inventory_ids.push_back(actor_inventory_id);
    draft.target_inventory_ids.push_back(actor_inventory_id);
    draft.changed_inventory_ids.push_back(actor_inventory_id);
    draft.changed_entity_ids.push_back(request.entity_id);
    draft.changed_cell_ids.push_back(entity->coord.value().cellId());
    draft.validated = true;

    return Result<InventoryTransferDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// PickupFromMap apply
// ---------------------------------------------------------------------------

Result<void> WorldInventoryRuntime::applyPickupFromMap(InventoryTransferDraft& draft) {
    const auto& request = draft.request;

    // Re-validate source location (prevent duplicate clicks / race)
    auto entity_res = world_port_.findEntity(request.entity_id);
    if (entity_res.is_error()) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "item_missing",
                      "Item no longer exists: " + request.entity_id));
    }
    const auto* entity = entity_res.value();
    if (entity->location_kind != WorldEntityLocationKind::OnMap) {
        return Result<void>::fail(
            makeError(ErrorCode::state_change_invalid, "conflict_location_state",
                      "Item is no longer on map: " + request.entity_id));
    }

    // Ensure actor inventory (apply phase may create)
    std::string actor_inventory_id = draft.target_inventory_ids[0];
    auto inv_ensure_res = findOrCreateActorInventory(request.actor_key);
    if (inv_ensure_res.is_error()) {
        return Result<void>::fail(inv_ensure_res.errors()[0]);
    }
    actor_inventory_id = inv_ensure_res.value();

    auto inv_ptr_res = findInventory(actor_inventory_id);
    if (inv_ptr_res.is_error()) {
        return Result<void>::fail(inv_ptr_res.errors()[0]);
    }
    auto* inv = inv_ptr_res.value();

    // Build stack_key from entity state
    std::string stack_key = makeStackKey(entity->entity_key, entity->state_keys, entity->numeric_states);

    // Determine if this is a partial or full pickup
    bool partial_pickup = (request.quantity < entity->quantity);

    if (partial_pickup) {
        // Partial pickup: reduce ground stack quantity, keep entity OnMap
        int new_ground_quantity = entity->quantity - request.quantity;
        auto set_res = world_port_.setEntityStackData(request.entity_id, new_ground_quantity, stack_key, entity->stackable);
        if (set_res.is_error()) {
            return Result<void>::fail(set_res.errors()[0]);
        }
        // Entity remains OnMap, item_locations_ stays unchanged
        draft.changed_entity_ids.push_back(request.entity_id);
    } else {
        // Full pickup: remove from map and set to InInventory
        auto remove_res = world_port_.removeEntityFromMap(request.entity_id);
        if (remove_res.is_error()) {
            return Result<void>::fail(remove_res.errors()[0]);
        }

        auto kind_res = world_port_.setEntityLocationKind(request.entity_id, WorldEntityLocationKind::InInventory);
        if (kind_res.is_error()) {
            return Result<void>::fail(kind_res.errors()[0]);
        }
    }

    // Try to merge with existing inventory stack
    bool merged = false;
    for (auto& entry : inv->entries) {
        if (entry.entity_key == entity->entity_key && entry.stack_key == stack_key && entry.stackable && entity->stackable) {
            entry.quantity += request.quantity;
            merged = true;
            break;
        }
    }

    if (!merged) {
        InventoryItemEntry entry;
        entry.entry_id = request.transfer_id + "_entry";
        entry.entity_key = entity->entity_key;
        entry.stack_key = stack_key;
        entry.quantity = request.quantity;
        entry.stackable = entity->stackable;
        entry.state_keys = entity->state_keys;
        entry.numeric_states = entity->numeric_states;

        if (partial_pickup) {
            // Partial pickup: new inventory carrier must have a distinct entity id
            // from the remaining ground stack to avoid ownership ambiguity.
            entry.entity_id = request.entity_id + "_pickup_" + std::to_string(state_version_);

            ItemLocationRef loc;
            loc.location_kind = InventoryLocationKind::InInventory;
            loc.inventory_id = actor_inventory_id;
            loc.owner_key = request.actor_key;
            item_locations_[entry.entity_id] = std::move(loc);
            draft.changed_entity_ids.push_back(entry.entity_id);
        } else {
            entry.entity_id = request.entity_id;
        }

        inv->entries.push_back(std::move(entry));
        inv->used_slots += 1;
    } else if (!partial_pickup) {
        // Full pickup merged: the original map entity is no longer needed as an independent instance.
        // Destroy it to avoid "InInventory entity without entry" inconsistency.
        auto destroy_res = world_port_.destroyEntity(request.entity_id);
        if (destroy_res.is_error()) {
            // Non-fatal for apply, but log warning
        }
    }

    // Update item location tracking
    if (partial_pickup) {
        // Ground entity stays OnMap, location unchanged
    } else {
        ItemLocationRef loc;
        loc.location_kind = InventoryLocationKind::InInventory;
        loc.inventory_id = actor_inventory_id;
        loc.owner_key = request.actor_key;
        item_locations_[request.entity_id] = std::move(loc);

        // If entity was merged and destroyed, erase its location tracking as well.
        auto entity_check = world_port_.findEntity(request.entity_id);
        if (entity_check.is_error()) {
            item_locations_.erase(request.entity_id);
        }
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// DropToMap validation
// ---------------------------------------------------------------------------

Result<InventoryTransferDraft> WorldInventoryRuntime::validateDropToMap(const InventoryTransferRequest& request) {
    InventoryTransferDraft draft;
    draft.draft_id = request.transfer_id + "_draft";
    draft.request = request;
    draft.validated = false;

    // Validate actor exists
    auto actor_res = world_port_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "actor_missing", "Actor not found: " + request.actor_key));
    }
    const auto* actor = actor_res.value();

    // Validate actor inventory exists (find only, do not create in validate)
    InventoryOwnerRef owner{InventoryOwnerKind::Actor, request.actor_key};
    std::string actor_inventory_id = makeInventoryId(owner);
    auto inv_ptr_res = findInventory(actor_inventory_id);

    // Validate item is in actor inventory (treat missing inventory as empty)
    InventoryItemEntry* target_entry = nullptr;
    if (inv_ptr_res.is_ok()) {
        for (auto& entry : inv_ptr_res.value()->entries) {
            if (entry.entity_id == request.entity_id || entry.entity_key == request.entity_key) {
                target_entry = &entry;
                break;
            }
        }
    }
    if (target_entry == nullptr) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "item_not_in_inventory",
                      "Item not in actor inventory: " + request.entity_id));
    }

    // Validate quantity
    if (request.quantity <= 0) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "quantity_invalid",
                      "Invalid quantity: " + std::to_string(request.quantity)));
    }
    if (request.quantity > target_entry->quantity) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_insufficient_cost, "quantity_insufficient",
                      "Not enough quantity to drop"));
    }

    // Determine drop coord (default actor current coord)
    WorldCellCoord drop_coord = actor->coord;
    auto drop_x_it = request.parameters.find("drop_x");
    auto drop_y_it = request.parameters.find("drop_y");
    auto layer_it = request.parameters.find("layer_key");
    if (drop_x_it != request.parameters.end() && drop_y_it != request.parameters.end()) {
        try {
            int dx = std::stoi(drop_x_it->second);
            int dy = std::stoi(drop_y_it->second);
            std::string layer = (layer_it != request.parameters.end()) ? layer_it->second : actor->coord.layer_key;
            drop_coord = WorldCellCoord{dx, dy, layer};
        } catch (...) {
            // keep actor coord
        }
    }

    // Validate target cell exists
    auto cell_res = world_port_.findCell(drop_coord);
    if (cell_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "target_cell_missing",
                      "Target cell not found: " + drop_coord.cellId()));
    }

    // Validate drop coord is same as actor (P45 minimal)
    if (drop_coord != actor->coord) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "drop_cell_invalid",
                      "Drop target must be actor's current cell in P45 minimal"));
    }

    // Build draft
    draft.source_inventory_ids.push_back(actor_inventory_id);
    draft.target_inventory_ids.push_back(actor_inventory_id);
    draft.source_entity_ids.push_back(request.entity_id);
    draft.changed_inventory_ids.push_back(actor_inventory_id);
    draft.changed_entity_ids.push_back(request.entity_id);
    draft.changed_cell_ids.push_back(drop_coord.cellId());
    draft.validated = true;

    return Result<InventoryTransferDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// DropToMap apply
// ---------------------------------------------------------------------------

Result<void> WorldInventoryRuntime::applyDropToMap(InventoryTransferDraft& draft) {
    const auto& request = draft.request;

    auto actor_res = world_port_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<void>::fail(actor_res.errors()[0]);
    }
    const auto* actor = actor_res.value();

    std::string actor_inventory_id = draft.source_inventory_ids[0];
    auto inv_ptr_res = findInventory(actor_inventory_id);
    if (inv_ptr_res.is_error()) {
        return Result<void>::fail(inv_ptr_res.errors()[0]);
    }
    auto* inv = inv_ptr_res.value();

    // Find and update/remove entry
    InventoryItemEntry* target_entry = nullptr;
    size_t target_index = 0;
    for (size_t i = 0; i < inv->entries.size(); ++i) {
        if (inv->entries[i].entity_id == request.entity_id || inv->entries[i].entity_key == request.entity_key) {
            target_entry = &inv->entries[i];
            target_index = i;
            break;
        }
    }
    if (target_entry == nullptr) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "item_not_in_inventory",
                      "Item no longer in inventory during apply"));
    }

    // Determine drop coord
    WorldCellCoord drop_coord = actor->coord;
    auto drop_x_it = request.parameters.find("drop_x");
    auto drop_y_it = request.parameters.find("drop_y");
    auto layer_it = request.parameters.find("layer_key");
    if (drop_x_it != request.parameters.end() && drop_y_it != request.parameters.end()) {
        try {
            int dx = std::stoi(drop_x_it->second);
            int dy = std::stoi(drop_y_it->second);
            std::string layer = (layer_it != request.parameters.end()) ? layer_it->second : actor->coord.layer_key;
            drop_coord = WorldCellCoord{dx, dy, layer};
        } catch (...) {
            // keep actor coord
        }
    }

    // Try to merge with existing ground stack
    auto ground_stack_res = findGroundStackEntity(drop_coord, target_entry->entity_key, target_entry->stack_key);
    bool merged_to_ground = false;
    if (ground_stack_res.is_ok() && target_entry->stackable) {
        // Merge into existing ground stack
        std::string ground_entity_id = ground_stack_res.value();
        auto stack_res = world_port_.findEntity(ground_entity_id);
        if (stack_res.is_ok()) {
            const auto* ground_ent = stack_res.value();
            int new_quantity = ground_ent->quantity + request.quantity;
            auto set_res = world_port_.setEntityStackData(ground_entity_id, new_quantity, target_entry->stack_key, target_entry->stackable);
            if (set_res.is_ok()) {
                merged_to_ground = true;
            }
        }
    }

    if (merged_to_ground) {
        // Only reduce inventory quantity, do not create new map entity
        if (request.quantity < target_entry->quantity) {
            target_entry->quantity -= request.quantity;
        } else {
            // Full drop merged to ground: erase entry and clean up source entity
            inv->entries.erase(inv->entries.begin() + target_index);
            inv->used_slots -= 1;

            // Destroy the source entity to prevent ghost InInventory state
            auto destroy_res = world_port_.destroyEntity(request.entity_id);
            if (destroy_res.is_error()) {
                // Non-fatal, but log warning
            }
            item_locations_.erase(request.entity_id);
        }

        // Ground stack was updated, include it in changed ids
        draft.changed_entity_ids.push_back(ground_stack_res.value());
    } else {
        // Copy entry data before potential erase
        InventoryItemEntry dropped_entry = *target_entry;

        // Partial drop: reduce quantity, create/update entity for dropped portion
        std::string dropped_entity_id = request.entity_id;
        if (request.quantity < target_entry->quantity) {
            target_entry->quantity -= request.quantity;
            // Generate a new entity id for the dropped portion
            dropped_entity_id = dropped_entry.entity_key + "_" + drop_coord.layer_key + "_" + std::to_string(drop_coord.x) + "_" + std::to_string(drop_coord.y) + "_drop_" + std::to_string(state_version_);
        } else {
            // Full drop: remove entry
            inv->entries.erase(inv->entries.begin() + target_index);
            inv->used_slots -= 1;
        }

        // Spawn entity on map with full explicit data (no guessing)
        auto spawn_res = world_port_.spawnEntityOnMap(
            dropped_entity_id,
            dropped_entry.entity_key,
            "entity." + dropped_entry.entity_key,
            drop_coord,
            request.quantity,
            dropped_entry.stack_key,
            dropped_entry.stackable,
            dropped_entry.state_keys,
            dropped_entry.numeric_states,
            {} // tag_keys
        );
        if (spawn_res.is_error()) {
            return Result<void>::fail(spawn_res.errors()[0]);
        }

        // Update item location tracking
        ItemLocationRef loc;
        loc.location_kind = InventoryLocationKind::OnMap;
        loc.coord = drop_coord;
        item_locations_[dropped_entity_id] = std::move(loc);

        // Add new entity to changed ids for projection
        draft.changed_entity_ids.push_back(dropped_entity_id);
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// ConsumeToNowhere validation
// ---------------------------------------------------------------------------

Result<InventoryTransferDraft> WorldInventoryRuntime::validateConsumeToNowhere(const InventoryTransferRequest& request) {
    InventoryTransferDraft draft;
    draft.draft_id = request.transfer_id + "_draft";
    draft.request = request;
    draft.validated = false;

    // Validate actor exists
    auto actor_res = world_port_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "actor_missing", "Actor not found: " + request.actor_key));
    }

    // Determine source inventory
    std::string source_inventory_id;
    if (!request.from.inventory_id.empty()) {
        source_inventory_id = request.from.inventory_id;
    } else {
        source_inventory_id = makeInventoryId(InventoryOwnerRef{InventoryOwnerKind::Actor, request.actor_key});
    }

    auto inv_ptr_res = findInventory(source_inventory_id);
    if (inv_ptr_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "inventory_missing", "Inventory not found: " + source_inventory_id));
    }
    auto* inv = inv_ptr_res.value();

    // Find target entry by entity_id or entity_key
    InventoryItemEntry* target_entry = nullptr;
    for (auto& entry : inv->entries) {
        if ((!request.entity_id.empty() && entry.entity_id == request.entity_id) ||
            (!request.entity_key.empty() && entry.entity_key == request.entity_key)) {
            target_entry = &entry;
            break;
        }
    }
    if (target_entry == nullptr) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "item_not_in_inventory",
                      "Item not in inventory: " + request.entity_id + "/" + request.entity_key));
    }

    // Validate quantity
    if (request.quantity <= 0) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "quantity_invalid",
                      "Invalid quantity: " + std::to_string(request.quantity)));
    }
    if (request.quantity > target_entry->quantity) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_insufficient_cost, "quantity_insufficient",
                      "Not enough quantity to consume"));
    }

    draft.source_inventory_ids.push_back(source_inventory_id);
    draft.source_entity_ids.push_back(target_entry->entity_id);
    draft.changed_inventory_ids.push_back(source_inventory_id);
    draft.changed_entity_ids.push_back(target_entry->entity_id);
    draft.validated = true;

    return Result<InventoryTransferDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// ConsumeToNowhere apply
// ---------------------------------------------------------------------------

Result<void> WorldInventoryRuntime::applyConsumeToNowhere(InventoryTransferDraft& draft) {
    const auto& request = draft.request;

    std::string source_inventory_id = draft.source_inventory_ids[0];
    auto inv_ptr_res = findInventory(source_inventory_id);
    if (inv_ptr_res.is_error()) {
        return Result<void>::fail(inv_ptr_res.errors()[0]);
    }
    auto* inv = inv_ptr_res.value();

    // Find and update/remove entry
    InventoryItemEntry* target_entry = nullptr;
    size_t target_index = 0;
    for (size_t i = 0; i < inv->entries.size(); ++i) {
        if (inv->entries[i].entity_id == draft.source_entity_ids[0]) {
            target_entry = &inv->entries[i];
            target_index = i;
            break;
        }
    }
    if (target_entry == nullptr) {
        return Result<void>::fail(
            makeError(ErrorCode::id_not_found, "item_not_in_inventory",
                      "Item no longer in inventory during apply"));
    }

    if (request.quantity < target_entry->quantity) {
        target_entry->quantity -= request.quantity;
    } else {
        // Full consume: remove entry
        inv->entries.erase(inv->entries.begin() + target_index);
        inv->used_slots -= 1;

        // Destroy source entity if it still exists as an independent instance
        auto destroy_res = world_port_.destroyEntity(target_entry->entity_id);
        if (destroy_res.is_error()) {
            // Non-fatal
        }
        item_locations_.erase(target_entry->entity_id);
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// SpawnToInventory validation
// ---------------------------------------------------------------------------

Result<InventoryTransferDraft> WorldInventoryRuntime::validateSpawnToInventory(const InventoryTransferRequest& request) {
    InventoryTransferDraft draft;
    draft.draft_id = request.transfer_id + "_draft";
    draft.request = request;
    draft.validated = false;

    // Validate actor exists
    auto actor_res = world_port_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::id_not_found, "actor_missing", "Actor not found: " + request.actor_key));
    }

    // Validate entity_key is provided
    if (request.entity_key.empty()) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "item_missing",
                      "SpawnToInventory requires entity_key"));
    }

    // Validate quantity
    if (request.quantity <= 0) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::command_invalid_argument, "quantity_invalid",
                      "Invalid quantity: " + std::to_string(request.quantity)));
    }

    // Determine target inventory
    std::string target_inventory_id;
    if (!request.to.inventory_id.empty()) {
        target_inventory_id = request.to.inventory_id;
    } else {
        target_inventory_id = makeInventoryId(InventoryOwnerRef{InventoryOwnerKind::Actor, request.actor_key});
    }

    // Check capacity (find only, do not create in validate)
    int used_slots = 0;
    int capacity_slots = 20;
    auto inv_ptr_res = findInventory(target_inventory_id);
    if (inv_ptr_res.is_ok()) {
        used_slots = inv_ptr_res.value()->used_slots;
        capacity_slots = inv_ptr_res.value()->capacity_slots;

        // Check if mergeable into existing stack
        std::string stack_key = request.entity_key + ":default";
        auto sk_it = request.parameters.find("stack_key");
        if (sk_it != request.parameters.end()) {
            stack_key = sk_it->second;
        }
        bool stackable = true;
        auto sb_it = request.parameters.find("stackable");
        if (sb_it != request.parameters.end()) {
            stackable = (sb_it->second == "true");
        }
        for (const auto& entry : inv_ptr_res.value()->entries) {
            if (entry.entity_key == request.entity_key && entry.stack_key == stack_key && entry.stackable && stackable) {
                // Mergeable: capacity not an issue
                used_slots = -1;
                break;
            }
        }
    }
    if (used_slots >= 0 && used_slots >= capacity_slots) {
        return Result<InventoryTransferDraft>::fail(
            makeError(ErrorCode::precondition_insufficient_cost, "capacity_exceeded",
                      "Inventory capacity exceeded"));
    }

    draft.target_inventory_ids.push_back(target_inventory_id);
    draft.changed_inventory_ids.push_back(target_inventory_id);
    draft.validated = true;

    return Result<InventoryTransferDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// SpawnToInventory apply
// ---------------------------------------------------------------------------

Result<void> WorldInventoryRuntime::applySpawnToInventory(InventoryTransferDraft& draft) {
    const auto& request = draft.request;

    // Ensure target inventory exists
    std::string target_inventory_id = draft.target_inventory_ids[0];
    auto inv_ensure_res = findOrCreateActorInventory(request.actor_key);
    if (inv_ensure_res.is_ok()) {
        target_inventory_id = inv_ensure_res.value();
    }

    auto inv_ptr_res = findInventory(target_inventory_id);
    if (inv_ptr_res.is_error()) {
        return Result<void>::fail(inv_ptr_res.errors()[0]);
    }
    auto* inv = inv_ptr_res.value();

    // Parse stack properties from parameters
    std::string stack_key = request.entity_key + ":default";
    auto sk_it = request.parameters.find("stack_key");
    if (sk_it != request.parameters.end()) {
        stack_key = sk_it->second;
    }
    bool stackable = true;
    auto sb_it = request.parameters.find("stackable");
    if (sb_it != request.parameters.end()) {
        stackable = (sb_it->second == "true");
    }
    std::vector<std::string> state_keys;
    auto st_it = request.parameters.find("state_keys");
    if (st_it != request.parameters.end()) {
        std::stringstream ss(st_it->second);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) state_keys.push_back(token);
        }
    }

    std::string display_name_key;
    auto dn_it = request.parameters.find("display_name_key");
    if (dn_it != request.parameters.end()) {
        display_name_key = dn_it->second;
    } else {
        display_name_key = "entity." + request.entity_key;
    }
    std::vector<std::string> tag_keys;
    auto tk_it = request.parameters.find("tag_keys");
    if (tk_it != request.parameters.end()) {
        std::stringstream ss(tk_it->second);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) tag_keys.push_back(token);
        }
    }

    // Try to merge with existing stack
    bool merged = false;
    for (auto& entry : inv->entries) {
        if (entry.entity_key == request.entity_key && entry.stack_key == stack_key && entry.stackable && stackable) {
            entry.quantity += request.quantity;
            merged = true;
            draft.changed_entity_ids.push_back(entry.entity_id);

            // Sync quantity to WorldEntityInstance
            auto set_res = world_port_.setEntityStackData(entry.entity_id, entry.quantity, entry.stack_key, entry.stackable);
            if (set_res.is_error()) {
                // Non-fatal, but log warning
            }
            break;
        }
    }

    if (!merged) {
        InventoryItemEntry entry;
        entry.entry_id = request.transfer_id + "_entry";
        entry.entity_key = request.entity_key;
        entry.stack_key = stack_key;
        entry.quantity = request.quantity;
        entry.stackable = stackable;
        entry.state_keys = std::move(state_keys);

        // Deterministic entity id: prefer request.entity_id, fallback to transfer-based id
        if (!request.entity_id.empty()) {
            entry.entity_id = request.entity_id;
        } else {
            entry.entity_id = request.entity_key + "_spawn_" + request.transfer_id + "_" + stack_key;
        }

        inv->entries.push_back(std::move(entry));
        inv->used_slots += 1;

        ItemLocationRef loc;
        loc.location_kind = InventoryLocationKind::InInventory;
        loc.inventory_id = target_inventory_id;
        loc.owner_key = request.actor_key;
        item_locations_[inv->entries.back().entity_id] = std::move(loc);

        draft.changed_entity_ids.push_back(inv->entries.back().entity_id);

        // Sync creation of WorldEntityInstance via location port
        auto spawn_res = world_port_.spawnEntityInInventory(
            inv->entries.back().entity_id,
            request.entity_key,
            display_name_key,
            request.actor_key,
            request.quantity,
            stack_key,
            stackable,
            inv->entries.back().state_keys,
            inv->entries.back().numeric_states,
            tag_keys);
        if (spawn_res.is_error()) {
            return Result<void>::fail(spawn_res.errors()[0]);
        }
    }

    return Result<void>::ok();
}

// ---------------------------------------------------------------------------
// Snapshot
// ---------------------------------------------------------------------------

Result<InventoryRuntimeSnapshot> WorldInventoryRuntime::snapshotForDebug() const {
    InventoryRuntimeSnapshot snapshot;
    snapshot.state_version = state_version_;
    snapshot.inventories = inventories_;
    snapshot.item_locations_by_entity_id = item_locations_;
    return Result<InventoryRuntimeSnapshot>::ok(std::move(snapshot));
}

} // namespace pathfinder::world_inventory
