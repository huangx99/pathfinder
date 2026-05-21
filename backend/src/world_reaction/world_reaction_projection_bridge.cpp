#include "pathfinder/world_reaction/world_reaction_projection_bridge.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include <algorithm>

namespace pathfinder::world_reaction {

using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::world_runtime::WorldProjectionAdapter;
using pathfinder::world_inventory::InventoryProjectionAdapter;

WorldProjectionPatchDto WorldReactionProjectionBridge::buildPatch(
    const WorldReactionResult& result,
    const std::string& viewer_actor_key,
    const pathfinder::world_runtime::IWorldRuntime& world_runtime,
    const pathfinder::world_inventory::IInventoryRuntime& inventory_runtime,
    uint64_t base_projection_version) {

    WorldProjectionPatchDto patch;
    if (base_projection_version > 0) {
        patch.base_projection_version = base_projection_version;
        patch.new_projection_version = base_projection_version + 1;
    } else {
        uint64_t combined_version = std::max(world_runtime.stateVersion(), inventory_runtime.stateVersion());
        patch.base_projection_version = combined_version;
        patch.new_projection_version = combined_version + 1;
    }
    patch.requires_full_refresh = false;

    // Cell patches
    WorldProjectionAdapter world_proj_adapter;
    auto cell_patches = world_proj_adapter.buildCellPatches(result.changed_cell_ids, viewer_actor_key, world_runtime);
    if (cell_patches.is_ok()) {
        patch.changed_cells = std::move(cell_patches.value());
    }

    // Entity patches
    auto entity_patches = world_proj_adapter.buildEntityPatches(result.changed_entity_ids, viewer_actor_key, world_runtime);
    if (entity_patches.is_ok()) {
        patch.changed_entities = std::move(entity_patches.value());
    }

    // Inventory patches
    InventoryProjectionAdapter inv_proj_adapter;
    auto inv_patches = inv_proj_adapter.buildInventoryPatches(result.changed_inventory_ids, viewer_actor_key, inventory_runtime);
    if (inv_patches.is_ok()) {
        patch.changed_inventories = std::move(inv_patches.value());
    }

    return patch;
}

} // namespace pathfinder::world_reaction
