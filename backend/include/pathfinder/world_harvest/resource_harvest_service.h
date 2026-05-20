#pragma once

#include "pathfinder/world_harvest/world_harvest_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"

namespace pathfinder::world_harvest {

// ---------------------------------------------------------------------------
// ResourceHarvestService - validate + apply orchestration
// ---------------------------------------------------------------------------

class ResourceHarvestService {
public:
    ResourceHarvestService(
        world_runtime::IWorldRuntime& world_runtime,
        world_inventory::IWorldEntityLocationPort& location_port,
        world_inventory::IInventoryRuntime& inventory_runtime);

    // Validate and build draft (read-only)
    pathfinder::foundation::Result<ResourceHarvestDraft> validate(
        const ResourceHarvestRequest& request) const;

    // Apply draft (write)
    ResourceHarvestResult apply(const ResourceHarvestDraft& draft);

    // Full validate + apply
    ResourceHarvestResult execute(const ResourceHarvestRequest& request);

private:
    world_runtime::IWorldRuntime& world_runtime_;
    world_inventory::IWorldEntityLocationPort& location_port_;
    world_inventory::IInventoryRuntime& inventory_runtime_;
};

} // namespace pathfinder::world_harvest
