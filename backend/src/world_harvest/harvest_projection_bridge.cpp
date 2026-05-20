#include "pathfinder/world_harvest/harvest_projection_bridge.h"

namespace pathfinder::world_harvest {

using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::PatchOp;

WorldProjectionPatchDto HarvestProjectionBridge::buildPatch(
    const ResourceHarvestResult& result,
    uint64_t base_projection_version)
{
    WorldProjectionPatchDto patch;
    patch.base_projection_version = base_projection_version;
    patch.new_projection_version = base_projection_version + 1;
    patch.requires_full_refresh = false;

    for (const auto& cell_id : result.changed_cell_ids) {
        WorldCellPatchDto cell_patch;
        cell_patch.cell_id = cell_id;
        cell_patch.op = PatchOp::Update;
        patch.changed_cells.push_back(std::move(cell_patch));
    }

    for (const auto& entity_id : result.changed_entity_ids) {
        WorldEntityPatchDto entity_patch;
        entity_patch.entity_id = entity_id;
        entity_patch.op = PatchOp::Add;
        patch.changed_entities.push_back(std::move(entity_patch));
    }

    return patch;
}

} // namespace pathfinder::world_harvest
