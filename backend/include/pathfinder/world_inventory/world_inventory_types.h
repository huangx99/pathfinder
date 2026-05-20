#pragma once

#include "pathfinder/foundation/result.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/world_command/world_command_types.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_inventory {

// ---------------------------------------------------------------------------
// 8. Core Enums
// ---------------------------------------------------------------------------

enum class InventoryOwnerKind {
    Unknown,
    Actor,
    Container,
    Camp,
    Tribe,
    Structure,
    Vehicle,
    System
};

std::string toString(InventoryOwnerKind kind);
pathfinder::foundation::Result<InventoryOwnerKind> inventoryOwnerKindFromString(const std::string& str);

enum class InventoryLocationKind {
    Unknown,
    OnMap,
    InInventory,
    InContainer,
    Equipped,
    Nowhere
};

std::string toString(InventoryLocationKind kind);
pathfinder::foundation::Result<InventoryLocationKind> inventoryLocationKindFromString(const std::string& str);

InventoryLocationKind mapWorldEntityLocationKind(world_runtime::WorldEntityLocationKind kind);
world_runtime::WorldEntityLocationKind mapInventoryLocationKind(InventoryLocationKind kind);

enum class InventoryTransferKind {
    Unknown,
    PickupFromMap,
    DropToMap,
    MoveInventoryToInventory,
    MoveInventoryToContainer,
    MoveContainerToInventory,
    EquipFromInventory,
    UnequipToInventory,
    ConsumeToNowhere,
    SpawnToInventory,
    SpawnToMap
};

std::string toString(InventoryTransferKind kind);
pathfinder::foundation::Result<InventoryTransferKind> inventoryTransferKindFromString(const std::string& str);

enum class InventoryFailureKind {
    None,
    ActorMissing,
    InventoryMissing,
    ItemMissing,
    ItemNotOnMap,
    ItemNotInInventory,
    ItemNotInContainer,
    TargetCellMissing,
    TargetTooFar,
    TargetNotVisible,
    QuantityInvalid,
    QuantityInsufficient,
    CapacityExceeded,
    ForbiddenByOwner,
    ContainerClosed,
    ItemNotStackable,
    ConflictLocationState,
    RuntimeMismatch,
    ContainerNotEmpty,
    ItemEquipped,
    DropCellInvalid
};

std::string toString(InventoryFailureKind kind);

enum class InventoryPatchKind {
    Unknown,
    InventoryCreated,
    InventoryUpdated,
    StackAdded,
    StackUpdated,
    StackRemoved,
    ItemMoved,
    ItemConsumed
};

std::string toString(InventoryPatchKind kind);

// ---------------------------------------------------------------------------
// 9. Core DTOs / Data Structures
// ---------------------------------------------------------------------------

struct InventoryOwnerRef {
    InventoryOwnerKind owner_kind = InventoryOwnerKind::Unknown;
    std::string owner_key;

    std::string toString() const;
    bool operator==(const InventoryOwnerRef& other) const;
};

struct InventoryItemEntry {
    std::string entry_id;
    std::string entity_id;
    std::string entity_key;
    std::string stack_key;
    int quantity = 1;
    bool stackable = true;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
};

struct InventoryRuntimeState {
    std::string inventory_id;
    InventoryOwnerRef owner;
    int capacity_slots = 20;
    int used_slots = 0;
    bool public_read = false;
    bool public_take = false;
    std::vector<InventoryItemEntry> entries;
};

struct ItemLocationRef {
    InventoryLocationKind location_kind = InventoryLocationKind::Unknown;
    std::optional<world_runtime::WorldCellCoord> coord;
    std::string inventory_id;
    std::string owner_key;
    std::string container_entity_id;
};

struct InventoryTransferRequest {
    std::string transfer_id;
    InventoryTransferKind transfer_kind = InventoryTransferKind::Unknown;
    std::string actor_key;
    std::string entity_id;
    std::string entity_key;
    int quantity = 1;
    ItemLocationRef from;
    ItemLocationRef to;
    std::map<std::string, std::string> parameters;
};

struct InventoryTransferResult {
    bool ok = false;
    InventoryFailureKind failure_kind = InventoryFailureKind::None;
    std::vector<std::string> changed_inventory_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_cell_ids;
    std::vector<world_command::WorldEventDto> events;
    std::vector<world_command::WorldStateDeltaDto> state_deltas;
};

struct InventoryRuntimeSnapshot {
    uint64_t state_version = 0;
    std::map<std::string, InventoryRuntimeState> inventories;
    std::map<std::string, ItemLocationRef> item_locations_by_entity_id;
};

// ---------------------------------------------------------------------------
// 18. Transaction / Draft model
// ---------------------------------------------------------------------------

struct InventoryTransferDraft {
    std::string draft_id;
    InventoryTransferRequest request;
    std::vector<std::string> source_inventory_ids;
    std::vector<std::string> target_inventory_ids;
    std::vector<std::string> source_entity_ids;
    std::vector<std::string> target_entity_ids;
    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_inventory_ids;
    bool validated = false;
    bool reserved = false;
};

// ---------------------------------------------------------------------------
// 18.2 SourceInventoryProof
// ---------------------------------------------------------------------------

struct SourceInventoryProof {
    std::string proof_id;
    std::string inventory_id;
    InventoryOwnerKind owner_kind = InventoryOwnerKind::Unknown;
    std::string owner_key;
    std::string entity_id;
    std::string entity_key;
    int quantity = 0;
    std::string reason_key;
};

// ---------------------------------------------------------------------------
// 18.6 ReservationToken
// ---------------------------------------------------------------------------

struct InventoryReservationToken {
    std::string reservation_id;
    std::string entity_id;
    std::string reserved_by_actor_key;
    uint64_t expire_world_tick = 0;
};

} // namespace pathfinder::world_inventory
