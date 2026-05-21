#pragma once

#include "pathfinder/world_inventory/iworld_inventory.h"
#include <memory>

namespace pathfinder::world_inventory {

class WorldInventoryRuntime final : public IInventoryRuntime {
public:
    explicit WorldInventoryRuntime(IWorldEntityLocationPort& world_port);

    foundation::Result<void> initialize() override;
    uint64_t stateVersion() const override;

    foundation::Result<InventoryRuntimeState*> findInventory(const std::string& inventory_id) override;
    foundation::Result<const InventoryRuntimeState*> findInventory(const std::string& inventory_id) const override;
    foundation::Result<std::string> ensureActorInventory(const std::string& actor_key) override;
    foundation::Result<std::string> ensureContainerInventory(const std::string& container_entity_id) override;

    foundation::Result<ItemLocationRef> findItemLocation(const std::string& entity_id) const override;
    foundation::Result<std::vector<InventoryItemEntry>> queryItems(
        const InventoryOwnerRef& owner, const std::string& entity_key) const override;

    foundation::Result<InventoryTransferResult> transfer(const InventoryTransferRequest& request) override;
    foundation::Result<InventoryRuntimeSnapshot> snapshotForDebug() const override;

private:
    IWorldEntityLocationPort& world_port_;
    uint64_t state_version_ = 0;
    std::map<std::string, InventoryRuntimeState> inventories_;
    std::map<std::string, ItemLocationRef> item_locations_;

    void incrementStateVersion();

    std::string makeInventoryId(const InventoryOwnerRef& owner) const;
    std::string makeStackKey(const std::string& entity_key, const std::vector<std::string>& state_keys,
                             const std::map<std::string, double>& numeric_states) const;

    foundation::Result<InventoryTransferDraft> validateTransfer(const InventoryTransferRequest& request);
    foundation::Result<void> applyTransfer(InventoryTransferDraft& draft);

    foundation::Result<InventoryTransferDraft> validatePickupFromMap(const InventoryTransferRequest& request);
    foundation::Result<InventoryTransferDraft> validateDropToMap(const InventoryTransferRequest& request);
    foundation::Result<InventoryTransferDraft> validateConsumeToNowhere(const InventoryTransferRequest& request);
    foundation::Result<InventoryTransferDraft> validateSpawnToInventory(const InventoryTransferRequest& request);

    foundation::Result<void> applyPickupFromMap(InventoryTransferDraft& draft);
    foundation::Result<void> applyDropToMap(InventoryTransferDraft& draft);
    foundation::Result<void> applyConsumeToNowhere(InventoryTransferDraft& draft);
    foundation::Result<void> applySpawnToInventory(InventoryTransferDraft& draft);

    foundation::Result<std::string> findOrCreateActorInventory(const std::string& actor_key);
    bool isContainerEntity(const world_runtime::WorldEntityInstance& entity) const;

    bool isAdjacent(const world_runtime::WorldCellCoord& a, const world_runtime::WorldCellCoord& b) const;
    bool isSameOrAdjacent(const world_runtime::WorldCellCoord& a, const world_runtime::WorldCellCoord& b) const;

    // Ground stack merge support
    foundation::Result<std::string> findGroundStackEntity(
        const world_runtime::WorldCellCoord& coord,
        const std::string& entity_key,
        const std::string& stack_key) const;
};

} // namespace pathfinder::world_inventory
