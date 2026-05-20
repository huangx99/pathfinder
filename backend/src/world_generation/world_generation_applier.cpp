#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_generation {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;

// ---------------------------------------------------------------------------
// WorldGenerationApplier
// ---------------------------------------------------------------------------

WorldGenerationApplier::WorldGenerationApplier(
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port)
    : world_runtime_(world_runtime), location_port_(location_port) {
}

WorldGenerationApplyResult WorldGenerationApplier::apply(const WorldGenerationResult& result) {
    WorldGenerationApplyResult apply_result;

    if (!result.ok) {
        apply_result.ok = false;
        apply_result.failure_kind = result.failure_kind;
        return apply_result;
    }

    // P46: Check region not already generated
    if (!result.cell_drafts.empty()) {
        std::string region_id = result.cell_drafts[0].region_id;
        if (world_runtime_.isRegionGenerated(region_id)) {
            apply_result.ok = false;
            apply_result.failure_kind = WorldGenerationFailureKind::RegionAlreadyGenerated;
            return apply_result;
        }
    }

    // Apply cells first (must exist before entities)
    auto cell_res = applyCells(result.cell_drafts);
    if (!cell_res.ok) {
        apply_result.ok = false;
        apply_result.failure_kind = cell_res.failure_kind;
        return apply_result;
    }
    apply_result.changed_cell_ids.insert(
        apply_result.changed_cell_ids.end(),
        cell_res.changed_cell_ids.begin(),
        cell_res.changed_cell_ids.end());
    apply_result.state_deltas.insert(
        apply_result.state_deltas.end(),
        cell_res.state_deltas.begin(),
        cell_res.state_deltas.end());
    apply_result.events.insert(
        apply_result.events.end(),
        cell_res.events.begin(),
        cell_res.events.end());

    // Apply resource nodes
    auto node_res = applyResourceNodes(result.resource_node_drafts);
    if (!node_res.ok) {
        apply_result.ok = false;
        apply_result.failure_kind = node_res.failure_kind;
        return apply_result;
    }
    apply_result.changed_cell_ids.insert(
        apply_result.changed_cell_ids.end(),
        node_res.changed_cell_ids.begin(),
        node_res.changed_cell_ids.end());
    apply_result.state_deltas.insert(
        apply_result.state_deltas.end(),
        node_res.state_deltas.begin(),
        node_res.state_deltas.end());
    apply_result.events.insert(
        apply_result.events.end(),
        node_res.events.begin(),
        node_res.events.end());

    // Apply spawn points (actors)
    auto spawn_res = applySpawnPoints(result.spawn_point_drafts);
    if (!spawn_res.ok) {
        apply_result.ok = false;
        apply_result.failure_kind = spawn_res.failure_kind;
        return apply_result;
    }
    apply_result.changed_cell_ids.insert(
        apply_result.changed_cell_ids.end(),
        spawn_res.changed_cell_ids.begin(),
        spawn_res.changed_cell_ids.end());
    apply_result.changed_entity_ids.insert(
        apply_result.changed_entity_ids.end(),
        spawn_res.changed_entity_ids.begin(),
        spawn_res.changed_entity_ids.end());

    // Apply entities (ground items) - cells must already exist
    auto entity_res = applyEntities(result.entity_drafts);
    if (!entity_res.ok) {
        apply_result.ok = false;
        apply_result.failure_kind = entity_res.failure_kind;
        return apply_result;
    }
    apply_result.changed_cell_ids.insert(
        apply_result.changed_cell_ids.end(),
        entity_res.changed_cell_ids.begin(),
        entity_res.changed_cell_ids.end());
    apply_result.changed_entity_ids.insert(
        apply_result.changed_entity_ids.end(),
        entity_res.changed_entity_ids.begin(),
        entity_res.changed_entity_ids.end());
    apply_result.state_deltas.insert(
        apply_result.state_deltas.end(),
        entity_res.state_deltas.begin(),
        entity_res.state_deltas.end());

    apply_result.events.insert(
        apply_result.events.end(),
        result.events.begin(),
        result.events.end());

    // Mark region as generated after successful apply
    if (!result.cell_drafts.empty()) {
        world_runtime_.markRegionGenerated(result.cell_drafts[0].region_id);
    }

    apply_result.ok = true;
    return apply_result;
}

WorldGenerationApplyResult WorldGenerationApplier::applyCells(const std::vector<GeneratedCellDraft>& cells) {
    WorldGenerationApplyResult result;

    for (const auto& cell : cells) {
        auto res = world_runtime_.createOrUpdateGeneratedCell(
            cell.coord,
            cell.terrain_key,
            cell.region_id,
            cell.blocks_movement,
            cell.movement_cost,
            cell.tag_keys);
        if (res.is_error()) {
            result.ok = false;
            result.failure_kind = WorldGenerationFailureKind::ApplyFailed;
            return result;
        }

        world_command::WorldStateDeltaDto delta;
        delta.delta_id = "gen_cell_" + cell.coord.cellId();
        delta.delta_kind = "CellCreated";
        delta.target_ref = cell.coord.cellId();
        delta.op = world_command::PatchOp::Add;
        delta.fields["terrain_key"] = cell.terrain_key;
        delta.fields["region_id"] = cell.region_id;
        delta.fields["blocks_movement"] = cell.blocks_movement ? "true" : "false";
        delta.fields["movement_cost"] = std::to_string(cell.movement_cost);
        result.state_deltas.push_back(std::move(delta));
        result.changed_cell_ids.push_back(cell.coord.cellId());
    }

    result.ok = true;
    return result;
}

WorldGenerationApplyResult WorldGenerationApplier::applyEntities(const std::vector<GeneratedEntityDraft>& entities) {
    WorldGenerationApplyResult result;

    for (const auto& draft : entities) {
        // P46: Ensure target cell exists before spawning to prevent ghost entities
        auto cell_check = world_runtime_.findCell(draft.coord);
        if (cell_check.is_error()) {
            result.ok = false;
            result.failure_kind = WorldGenerationFailureKind::ApplyFailed;
            return result;
        }

        // Use IWorldEntityLocationPort::spawnEntityOnMap for P45 compliance
        auto spawn_res = location_port_.spawnEntityOnMap(
            draft.entity_id,
            draft.entity_key,
            draft.display_name_key,
            draft.coord,
            draft.quantity,
            draft.stack_key,
            draft.stackable,
            draft.state_keys,
            draft.numeric_states,
            draft.tag_keys);

        if (spawn_res.is_error()) {
            result.ok = false;
            result.failure_kind = WorldGenerationFailureKind::ApplyFailed;
            return result;
        }

        result.changed_cell_ids.push_back(draft.coord.cellId());
        result.changed_entity_ids.push_back(draft.entity_id);

        world_command::WorldStateDeltaDto delta;
        delta.delta_id = "gen_entity_" + draft.entity_id;
        delta.delta_kind = "EntitySpawned";
        delta.target_ref = draft.entity_id;
        delta.op = world_command::PatchOp::Add;
        delta.fields["entity_key"] = draft.entity_key;
        delta.fields["coord"] = draft.coord.cellId();
        delta.fields["quantity"] = std::to_string(draft.quantity);
        result.state_deltas.push_back(std::move(delta));
    }

    result.ok = true;
    return result;
}

WorldGenerationApplyResult WorldGenerationApplier::applyResourceNodes(const std::vector<GeneratedResourceNodeDraft>& nodes) {
    WorldGenerationApplyResult result;

    for (const auto& draft : nodes) {
        // P46: reject resource nodes on missing cells to prevent ghost nodes
        auto cell_check = world_runtime_.findCell(draft.coord);
        if (cell_check.is_error()) {
            result.ok = false;
            result.failure_kind = WorldGenerationFailureKind::ApplyFailed;
            return result;
        }

        world_runtime::WorldResourceNodeRuntime node;
        node.node_id = draft.node_id;
        node.resource_key = draft.resource_key;
        node.coord = draft.coord;
        node.node_kind_str = toString(draft.node_kind);
        node.node_state_str = toString(draft.state);
        node.remaining_charges = draft.remaining_charges;
        node.max_charges = draft.max_charges;
        node.required_action_key = draft.required_action_key;
        node.required_tool_key = draft.required_tool_key;
        node.output_entity_keys = draft.output_entity_keys;
        node.tag_keys = draft.tag_keys;

        auto res = world_runtime_.upsertGeneratedResourceNode(node);
        if (res.is_error()) {
            result.ok = false;
            result.failure_kind = WorldGenerationFailureKind::ApplyFailed;
            return result;
        }

        world_command::WorldStateDeltaDto delta;
        delta.delta_id = "gen_node_" + draft.node_id;
        delta.delta_kind = "ResourceNodeCreated";
        delta.target_ref = draft.node_id;
        delta.op = world_command::PatchOp::Add;
        delta.fields["resource_key"] = draft.resource_key;
        delta.fields["coord"] = draft.coord.cellId();
        delta.fields["charges"] = std::to_string(draft.remaining_charges);
        result.state_deltas.push_back(std::move(delta));
        result.changed_cell_ids.push_back(draft.coord.cellId());
    }

    result.ok = true;
    return result;
}

WorldGenerationApplyResult WorldGenerationApplier::applySpawnPoints(const std::vector<GeneratedSpawnPointDraft>& spawns) {
    WorldGenerationApplyResult result;

    for (const auto& spawn : spawns) {
        if (spawn.spawn_kind == SpawnPointKind::PlayerStart) {
            // Player spawn point: we expect WorldGridRuntime to already handle player creation
            // in generateInitialWorld. P46 applier records this as a delta.
            world_command::WorldStateDeltaDto delta;
            delta.delta_id = "gen_spawn_" + spawn.spawn_id;
            delta.delta_kind = "ActorSpawned";
            delta.target_ref = spawn.actor_key;
            delta.op = world_command::PatchOp::Add;
            delta.fields["spawn_kind"] = "PlayerStart";
            delta.fields["coord"] = spawn.coord.cellId();
            result.state_deltas.push_back(std::move(delta));
            result.changed_cell_ids.push_back(spawn.coord.cellId());
        }
    }

    result.ok = true;
    return result;
}

} // namespace pathfinder::world_generation
