#include "pathfinder/world_inventory/world_inventory_types.h"
#include <algorithm>

namespace pathfinder::world_inventory {

using pathfinder::foundation::Result;

// ---------------------------------------------------------------------------
// InventoryOwnerKind
// ---------------------------------------------------------------------------

std::string toString(InventoryOwnerKind kind) {
    switch (kind) {
        case InventoryOwnerKind::Actor: return "actor";
        case InventoryOwnerKind::Container: return "container";
        case InventoryOwnerKind::Camp: return "camp";
        case InventoryOwnerKind::Tribe: return "tribe";
        case InventoryOwnerKind::Structure: return "structure";
        case InventoryOwnerKind::Vehicle: return "vehicle";
        case InventoryOwnerKind::System: return "system";
        default: return "unknown";
    }
}

Result<InventoryOwnerKind> inventoryOwnerKindFromString(const std::string& str) {
    if (str == "actor") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Actor);
    if (str == "container") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Container);
    if (str == "camp") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Camp);
    if (str == "tribe") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Tribe);
    if (str == "structure") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Structure);
    if (str == "vehicle") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Vehicle);
    if (str == "system") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::System);
    if (str == "unknown") return Result<InventoryOwnerKind>::ok(InventoryOwnerKind::Unknown);
    return Result<InventoryOwnerKind>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::command_invalid_argument,
            "invalid_owner_kind",
            "Unknown inventory owner kind: " + str));
}

// ---------------------------------------------------------------------------
// InventoryLocationKind
// ---------------------------------------------------------------------------

std::string toString(InventoryLocationKind kind) {
    switch (kind) {
        case InventoryLocationKind::OnMap: return "on_map";
        case InventoryLocationKind::InInventory: return "in_inventory";
        case InventoryLocationKind::InContainer: return "in_container";
        case InventoryLocationKind::Equipped: return "equipped";
        case InventoryLocationKind::Nowhere: return "nowhere";
        default: return "unknown";
    }
}

Result<InventoryLocationKind> inventoryLocationKindFromString(const std::string& str) {
    if (str == "on_map") return Result<InventoryLocationKind>::ok(InventoryLocationKind::OnMap);
    if (str == "in_inventory") return Result<InventoryLocationKind>::ok(InventoryLocationKind::InInventory);
    if (str == "in_container") return Result<InventoryLocationKind>::ok(InventoryLocationKind::InContainer);
    if (str == "equipped") return Result<InventoryLocationKind>::ok(InventoryLocationKind::Equipped);
    if (str == "nowhere") return Result<InventoryLocationKind>::ok(InventoryLocationKind::Nowhere);
    if (str == "unknown") return Result<InventoryLocationKind>::ok(InventoryLocationKind::Unknown);
    return Result<InventoryLocationKind>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::command_invalid_argument,
            "invalid_location_kind",
            "Unknown inventory location kind: " + str));
}

InventoryLocationKind mapWorldEntityLocationKind(world_runtime::WorldEntityLocationKind kind) {
    switch (kind) {
        case world_runtime::WorldEntityLocationKind::OnMap: return InventoryLocationKind::OnMap;
        case world_runtime::WorldEntityLocationKind::InInventory: return InventoryLocationKind::InInventory;
        case world_runtime::WorldEntityLocationKind::InContainer: return InventoryLocationKind::InContainer;
        case world_runtime::WorldEntityLocationKind::Equipped: return InventoryLocationKind::Equipped;
        case world_runtime::WorldEntityLocationKind::Nowhere: return InventoryLocationKind::Nowhere;
        default: return InventoryLocationKind::Unknown;
    }
}

world_runtime::WorldEntityLocationKind mapInventoryLocationKind(InventoryLocationKind kind) {
    switch (kind) {
        case InventoryLocationKind::OnMap: return world_runtime::WorldEntityLocationKind::OnMap;
        case InventoryLocationKind::InInventory: return world_runtime::WorldEntityLocationKind::InInventory;
        case InventoryLocationKind::InContainer: return world_runtime::WorldEntityLocationKind::InContainer;
        case InventoryLocationKind::Equipped: return world_runtime::WorldEntityLocationKind::Equipped;
        case InventoryLocationKind::Nowhere: return world_runtime::WorldEntityLocationKind::Nowhere;
        default: return world_runtime::WorldEntityLocationKind::Nowhere;
    }
}

// ---------------------------------------------------------------------------
// InventoryTransferKind
// ---------------------------------------------------------------------------

std::string toString(InventoryTransferKind kind) {
    switch (kind) {
        case InventoryTransferKind::PickupFromMap: return "pickup_from_map";
        case InventoryTransferKind::DropToMap: return "drop_to_map";
        case InventoryTransferKind::MoveInventoryToInventory: return "move_inventory_to_inventory";
        case InventoryTransferKind::MoveInventoryToContainer: return "move_inventory_to_container";
        case InventoryTransferKind::MoveContainerToInventory: return "move_container_to_inventory";
        case InventoryTransferKind::EquipFromInventory: return "equip_from_inventory";
        case InventoryTransferKind::UnequipToInventory: return "unequip_to_inventory";
        case InventoryTransferKind::ConsumeToNowhere: return "consume_to_nowhere";
        case InventoryTransferKind::SpawnToInventory: return "spawn_to_inventory";
        case InventoryTransferKind::SpawnToMap: return "spawn_to_map";
        default: return "unknown";
    }
}

Result<InventoryTransferKind> inventoryTransferKindFromString(const std::string& str) {
    if (str == "pickup_from_map") return Result<InventoryTransferKind>::ok(InventoryTransferKind::PickupFromMap);
    if (str == "drop_to_map") return Result<InventoryTransferKind>::ok(InventoryTransferKind::DropToMap);
    if (str == "move_inventory_to_inventory") return Result<InventoryTransferKind>::ok(InventoryTransferKind::MoveInventoryToInventory);
    if (str == "move_inventory_to_container") return Result<InventoryTransferKind>::ok(InventoryTransferKind::MoveInventoryToContainer);
    if (str == "move_container_to_inventory") return Result<InventoryTransferKind>::ok(InventoryTransferKind::MoveContainerToInventory);
    if (str == "equip_from_inventory") return Result<InventoryTransferKind>::ok(InventoryTransferKind::EquipFromInventory);
    if (str == "unequip_to_inventory") return Result<InventoryTransferKind>::ok(InventoryTransferKind::UnequipToInventory);
    if (str == "consume_to_nowhere") return Result<InventoryTransferKind>::ok(InventoryTransferKind::ConsumeToNowhere);
    if (str == "spawn_to_inventory") return Result<InventoryTransferKind>::ok(InventoryTransferKind::SpawnToInventory);
    if (str == "spawn_to_map") return Result<InventoryTransferKind>::ok(InventoryTransferKind::SpawnToMap);
    if (str == "unknown") return Result<InventoryTransferKind>::ok(InventoryTransferKind::Unknown);
    return Result<InventoryTransferKind>::fail(
        pathfinder::foundation::makeError(
            pathfinder::foundation::ErrorCode::command_invalid_argument,
            "invalid_transfer_kind",
            "Unknown inventory transfer kind: " + str));
}

// ---------------------------------------------------------------------------
// InventoryFailureKind
// ---------------------------------------------------------------------------

std::string toString(InventoryFailureKind kind) {
    switch (kind) {
        case InventoryFailureKind::ActorMissing: return "actor_missing";
        case InventoryFailureKind::InventoryMissing: return "inventory_missing";
        case InventoryFailureKind::ItemMissing: return "item_missing";
        case InventoryFailureKind::ItemNotOnMap: return "item_not_on_map";
        case InventoryFailureKind::ItemNotInInventory: return "item_not_in_inventory";
        case InventoryFailureKind::ItemNotInContainer: return "item_not_in_container";
        case InventoryFailureKind::TargetCellMissing: return "target_cell_missing";
        case InventoryFailureKind::TargetTooFar: return "target_too_far";
        case InventoryFailureKind::TargetNotVisible: return "target_not_visible";
        case InventoryFailureKind::QuantityInvalid: return "quantity_invalid";
        case InventoryFailureKind::QuantityInsufficient: return "quantity_insufficient";
        case InventoryFailureKind::CapacityExceeded: return "capacity_exceeded";
        case InventoryFailureKind::ForbiddenByOwner: return "forbidden_by_owner";
        case InventoryFailureKind::ContainerClosed: return "container_closed";
        case InventoryFailureKind::ItemNotStackable: return "item_not_stackable";
        case InventoryFailureKind::ConflictLocationState: return "conflict_location_state";
        case InventoryFailureKind::RuntimeMismatch: return "runtime_mismatch";
        case InventoryFailureKind::ContainerNotEmpty: return "container_not_empty";
        case InventoryFailureKind::ItemEquipped: return "item_equipped";
        case InventoryFailureKind::DropCellInvalid: return "drop_cell_invalid";
        default: return "none";
    }
}

// ---------------------------------------------------------------------------
// InventoryPatchKind
// ---------------------------------------------------------------------------

std::string toString(InventoryPatchKind kind) {
    switch (kind) {
        case InventoryPatchKind::InventoryCreated: return "inventory_created";
        case InventoryPatchKind::InventoryUpdated: return "inventory_updated";
        case InventoryPatchKind::StackAdded: return "stack_added";
        case InventoryPatchKind::StackUpdated: return "stack_updated";
        case InventoryPatchKind::StackRemoved: return "stack_removed";
        case InventoryPatchKind::ItemMoved: return "item_moved";
        case InventoryPatchKind::ItemConsumed: return "item_consumed";
        default: return "unknown";
    }
}

// ---------------------------------------------------------------------------
// InventoryOwnerRef
// ---------------------------------------------------------------------------

std::string InventoryOwnerRef::toString() const {
    return ::pathfinder::world_inventory::toString(owner_kind) + ":" + owner_key;
}

bool InventoryOwnerRef::operator==(const InventoryOwnerRef& other) const {
    return owner_kind == other.owner_kind && owner_key == other.owner_key;
}

} // namespace pathfinder::world_inventory
