#pragma once

#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/world_command/world_command_types.h"
#include "pathfinder/foundation/result.h"
#include <vector>

namespace pathfinder::world_inventory {

class InventoryProjectionAdapter {
public:
    foundation::Result<std::vector<world_command::InventoryPatchDto>> buildInventoryPatches(
        const std::vector<std::string>& changed_inventory_ids,
        const std::string& viewer_actor_key,
        const IInventoryRuntime& inventory_runtime) const;
};

} // namespace pathfinder::world_inventory
