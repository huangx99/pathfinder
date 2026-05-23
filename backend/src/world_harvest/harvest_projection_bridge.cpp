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

    if (result.has_node_after) {
        WorldEntityPatchDto node_patch;
        node_patch.entity_id = result.node_after.node_id;
        node_patch.op = result.node_after.remaining_charges <= 0 || result.node_after.node_state_str == "Depleted"
            ? PatchOp::Remove
            : PatchOp::Update;
        if (node_patch.op != PatchOp::Remove) {
            node_patch.fields["entity_key"] = result.node_after.resource_key;
            node_patch.fields["display_name_key"] = "object." + result.node_after.resource_key + ".name";
            node_patch.fields["x"] = std::to_string(result.node_after.coord.x);
            node_patch.fields["y"] = std::to_string(result.node_after.coord.y);
            node_patch.fields["layer_key"] = result.node_after.coord.layer_key;
            node_patch.fields["resource_node"] = "true";
            node_patch.fields["remaining_charges"] = std::to_string(result.node_after.remaining_charges);
        }
        patch.changed_entities.push_back(std::move(node_patch));
    }

    for (const auto& output : result.output_drafts) {
        WorldEntityPatchDto entity_patch;
        entity_patch.entity_id = output.entity_id;
        entity_patch.op = PatchOp::Add;
        entity_patch.fields["entity_key"] = output.entity_key;
        entity_patch.fields["display_name_key"] = output.display_name_key;
        entity_patch.fields["x"] = std::to_string(output.coord.x);
        entity_patch.fields["y"] = std::to_string(output.coord.y);
        entity_patch.fields["layer_key"] = output.coord.layer_key;
        entity_patch.fields["quantity"] = std::to_string(output.quantity);
        patch.changed_entities.push_back(std::move(entity_patch));
    }

    return patch;
}

} // namespace pathfinder::world_harvest
