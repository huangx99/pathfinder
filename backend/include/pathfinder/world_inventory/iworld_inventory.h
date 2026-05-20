#pragma once

#include "pathfinder/world_inventory/world_inventory_types.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include "pathfinder/foundation/result.h"

namespace pathfinder::world_inventory {

// ---------------------------------------------------------------------------
// 10.1 IInventoryRuntime
// ---------------------------------------------------------------------------

class IInventoryRuntime {
public:
    virtual ~IInventoryRuntime() = default;

    virtual foundation::Result<void> initialize() = 0;
    virtual uint64_t stateVersion() const = 0;

    virtual foundation::Result<InventoryRuntimeState*> findInventory(const std::string& inventory_id) = 0;
    virtual foundation::Result<const InventoryRuntimeState*> findInventory(const std::string& inventory_id) const = 0;
    virtual foundation::Result<std::string> ensureActorInventory(const std::string& actor_key) = 0;
    virtual foundation::Result<std::string> ensureContainerInventory(const std::string& container_entity_id) = 0;

    virtual foundation::Result<ItemLocationRef> findItemLocation(const std::string& entity_id) const = 0;
    virtual foundation::Result<std::vector<InventoryItemEntry>> queryItems(
        const InventoryOwnerRef& owner, const std::string& entity_key) const = 0;

    virtual foundation::Result<InventoryTransferResult> transfer(const InventoryTransferRequest& request) = 0;
    virtual foundation::Result<InventoryRuntimeSnapshot> snapshotForDebug() const = 0;
};

// ---------------------------------------------------------------------------
// 10.3 IWorldEntityLocationPort
// ---------------------------------------------------------------------------

class IWorldEntityLocationPort {
public:
    virtual ~IWorldEntityLocationPort() = default;

    virtual foundation::Result<const world_runtime::WorldEntityInstance*> findEntity(const std::string& entity_id) const = 0;
    virtual foundation::Result<const world_runtime::WorldActorRuntime*> findActor(const std::string& actor_key) const = 0;
    virtual foundation::Result<const world_runtime::WorldCellRuntime*> findCell(const world_runtime::WorldCellCoord& coord) const = 0;

    virtual foundation::Result<void> removeEntityFromMap(const std::string& entity_id) = 0;
    virtual foundation::Result<void> placeEntityOnMap(const std::string& entity_id, const world_runtime::WorldCellCoord& coord) = 0;
    virtual foundation::Result<void> setEntityLocationKind(const std::string& entity_id, world_runtime::WorldEntityLocationKind kind) = 0;
    virtual foundation::Result<void> setEntityStackData(const std::string& entity_id, int quantity, const std::string& stack_key, bool stackable) = 0;
    virtual foundation::Result<void> destroyEntity(const std::string& entity_id) = 0;
    virtual bool isCellVisibleToActor(const std::string& actor_key, const world_runtime::WorldCellCoord& coord) const = 0;
    virtual world_runtime::WorldCellVisibility getCellVisibilityForActor(const std::string& actor_key, const std::string& cell_id) const = 0;
    virtual foundation::Result<void> spawnEntityOnMap(
        const std::string& entity_id,
        const std::string& entity_key,
        const std::string& display_name_key,
        const world_runtime::WorldCellCoord& coord,
        int quantity,
        const std::string& stack_key,
        bool stackable,
        const std::vector<std::string>& state_keys,
        const std::map<std::string, double>& numeric_states,
        const std::vector<std::string>& tag_keys) = 0;
};

} // namespace pathfinder::world_inventory
