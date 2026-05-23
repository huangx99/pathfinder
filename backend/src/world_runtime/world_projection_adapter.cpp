#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"

namespace pathfinder::world_runtime {

using pathfinder::foundation::Result;

Result<std::vector<world_command::WorldCellPatchDto>> WorldProjectionAdapter::buildCellPatches(
    const std::vector<std::string>& changed_cell_ids,
    const std::string& viewer_actor_key,
    const IWorldRuntime& runtime) const {

    std::vector<world_command::WorldCellPatchDto> patches;

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime);
    if (!grid) {
        return Result<std::vector<world_command::WorldCellPatchDto>>::ok(std::move(patches));
    }

    for (const auto& cell_id : changed_cell_ids) {
        auto visibility = grid->getCellVisibility(viewer_actor_key, cell_id);
        if (visibility == WorldCellVisibility::Unknown) {
            continue; // Never send Unknown cells
        }

        // Parse cell_id: layer:x:y
        size_t first_colon = cell_id.find(':');
        size_t second_colon = cell_id.find(':', first_colon + 1);
        if (first_colon == std::string::npos || second_colon == std::string::npos) continue;

        std::string layer = cell_id.substr(0, first_colon);
        int x = std::stoi(cell_id.substr(first_colon + 1, second_colon - first_colon - 1));
        int y = std::stoi(cell_id.substr(second_colon + 1));

        WorldCellCoord coord{x, y, layer};
        auto cell_result = runtime.findCell(coord);
        if (cell_result.is_error()) continue;

        const WorldCellRuntime* cell = cell_result.value();
        world_command::WorldCellPatchDto patch;
        patch.cell_id = cell_id;
        patch.op = world_command::PatchOp::Update;
        patch.fields["x"] = std::to_string(cell->coord.x);
        patch.fields["y"] = std::to_string(cell->coord.y);
        patch.fields["layer_key"] = cell->coord.layer_key;
        patch.fields["terrain_key"] = cell->terrain_key;
        patch.fields["region_id"] = cell->region_id;
        patch.fields["visibility"] = toString(visibility);
        patch.fields["blocks_movement"] = cell->blocks_movement ? "true" : "false";
        patch.fields["movement_cost"] = std::to_string(cell->movement_cost);

        // For Visible, include entity refs; for Discovered, omit current entities
        if (visibility == WorldCellVisibility::Visible) {
            std::string entity_list;
            for (const auto& eid : cell->entity_ids) {
                if (!entity_list.empty()) entity_list += ",";
                entity_list += eid;
            }
            if (!entity_list.empty()) {
                patch.fields["entity_ids"] = entity_list;
            }
        }

        std::string tags;
        for (const auto& tag : cell->tag_keys) {
            if (!tags.empty()) tags += ",";
            tags += tag;
        }
        if (!tags.empty()) {
            patch.fields["tag_keys"] = tags;
        }

        patches.push_back(std::move(patch));
    }

    return Result<std::vector<world_command::WorldCellPatchDto>>::ok(std::move(patches));
}

Result<std::vector<world_command::WorldEntityPatchDto>> WorldProjectionAdapter::buildEntityPatches(
    const std::vector<std::string>& changed_entity_ids,
    const std::string& viewer_actor_key,
    const IWorldRuntime& runtime) const {

    std::vector<world_command::WorldEntityPatchDto> patches;

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime);

    for (const auto& entity_id : changed_entity_ids) {
        auto ent_result = runtime.findEntity(entity_id);
        if (ent_result.is_error()) {
            world_command::WorldEntityPatchDto patch;
            patch.entity_id = entity_id;
            patch.op = world_command::PatchOp::Remove;
            patches.push_back(std::move(patch));
            continue;
        }

        const WorldEntityInstance* entity = ent_result.value();
        if (!entity->coord || entity->location_kind != WorldEntityLocationKind::OnMap) {
            world_command::WorldEntityPatchDto patch;
            patch.entity_id = entity_id;
            patch.op = world_command::PatchOp::Remove;
            patches.push_back(std::move(patch));
            continue;
        }

        // Only send if cell is visible to viewer
        if (grid) {
            auto visibility = grid->getCellVisibility(viewer_actor_key, entity->coord->cellId());
            if (visibility != WorldCellVisibility::Visible) {
                continue;
            }
        }

        world_command::WorldEntityPatchDto patch;
        patch.entity_id = entity_id;
        patch.op = world_command::PatchOp::Update;
        patch.fields["entity_id"] = entity->entity_id;
        patch.fields["entity_key"] = entity->entity_key;
        patch.fields["display_name_key"] = entity->display_name_key;
        patch.fields["x"] = std::to_string(entity->coord->x);
        patch.fields["y"] = std::to_string(entity->coord->y);
        patch.fields["layer_key"] = entity->coord->layer_key;
        patch.fields["location_kind"] = toString(entity->location_kind);
        patch.fields["visible"] = entity->visible_by_default ? "true" : "false";
        patch.fields["blocks_movement"] = entity->blocks_movement ? "true" : "false";
        auto snap_res = runtime.snapshotForDebug();
        if (snap_res.is_ok()) {
            for (const auto& [actor_key, actor] : snap_res.value().actors) {
                if (actor.entity_id == entity->entity_id) {
                    patch.fields["actor_key"] = actor_key;
                    break;
                }
            }
        }

        std::string tags;
        for (const auto& tag : entity->tag_keys) {
            if (!tags.empty()) tags += ",";
            tags += tag;
        }
        if (!tags.empty()) {
            patch.fields["tag_keys"] = tags;
        }

        patches.push_back(std::move(patch));
    }

    return Result<std::vector<world_command::WorldEntityPatchDto>>::ok(std::move(patches));
}

} // namespace pathfinder::world_runtime
