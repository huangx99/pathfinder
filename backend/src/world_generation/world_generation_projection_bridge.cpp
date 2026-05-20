#include "pathfinder/world_generation/world_generation_projection_bridge.h"

namespace pathfinder::world_generation {

// ---------------------------------------------------------------------------
// WorldGenerationProjectionBridge
// ---------------------------------------------------------------------------

world_command::WorldProjectionPatchDto WorldGenerationProjectionBridge::buildPatch(
    const WorldGenerationApplyResult& apply_result,
    uint64_t base_projection_version) {
    world_command::WorldProjectionPatchDto patch;
    patch.base_projection_version = base_projection_version;
    patch.new_projection_version = base_projection_version + 1;
    patch.requires_full_refresh = false;

    for (const auto& cell_id : apply_result.changed_cell_ids) {
        world_command::WorldCellPatchDto cell_patch;
        cell_patch.cell_id = cell_id;
        cell_patch.op = world_command::PatchOp::Add;
        patch.changed_cells.push_back(std::move(cell_patch));
    }

    for (const auto& entity_id : apply_result.changed_entity_ids) {
        world_command::WorldEntityPatchDto entity_patch;
        entity_patch.entity_id = entity_id;
        entity_patch.op = world_command::PatchOp::Add;
        patch.changed_entities.push_back(std::move(entity_patch));
    }

    return patch;
}

std::vector<world_command::WorldEventDto> WorldGenerationProjectionBridge::buildEvents(
    const WorldGenerationResult& result) {
    return result.events;
}

} // namespace pathfinder::world_generation
