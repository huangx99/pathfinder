#include "pathfinder/world_region_state/world_grid_runtime_apply_port.h"
#include "pathfinder/world_generation/world_region_math.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <set>

namespace pathfinder::world_region_state {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_generation::WorldRegionMath;

WorldGridRuntimeApplyPort::WorldGridRuntimeApplyPort(
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port)
    : world_runtime_(world_runtime)
    , location_port_(location_port) {
}

Result<void> WorldGridRuntimeApplyPort::validateNoActiveConflict(
    const world_generation::WorldRegionKey& region_key) {

    if (world_runtime_.isRegionGenerated(region_key.regionRuntimeId())) {
        return Result<void>::fail(
            makeError(ErrorCode::rule_conflict, "runtime_conflict_active_region",
                      "Region " + region_key.regionRuntimeId() + " is already active in runtime. "
                      "Restore refused to avoid overwriting active state."));
    }
    return Result<void>::ok();
}

Result<void> WorldGridRuntimeApplyPort::validateEntityOwnership(
    const std::vector<WorldRegionEntityStateRecord>& entities) {
    for (const auto& entity : entities) {
        if (entity.location_kind != world_runtime::WorldEntityLocationKind::OnMap) {
            return Result<void>::fail(
                makeError(ErrorCode::rule_conflict, "ownership_violation",
                          "Entity " + entity.entity_id + " has location_kind " +
                          world_runtime::toString(entity.location_kind) + ", expected OnMap"));
        }
    }
    return Result<void>::ok();
}

Result<WorldRegionApplyPlan> WorldGridRuntimeApplyPort::planRestore(
    const WorldRegionSnapshot& snapshot) {

    WorldRegionApplyPlan plan;
    plan.region_key = snapshot.header.region_key;

    // Check no active conflict (region can be active, we'll clear it)
    auto conflict_res = validateNoActiveConflict(snapshot.header.region_key);
    if (conflict_res.is_error()) {
        return Result<WorldRegionApplyPlan>::fail(conflict_res.errors());
    }

    // Check OnMap ownership
    auto ownership_res = validateEntityOwnership(snapshot.on_map_entities);
    if (ownership_res.is_error()) {
        return Result<WorldRegionApplyPlan>::fail(ownership_res.errors());
    }

    // Check entity ID conflicts with existing non-region entities
    for (const auto& entity : snapshot.on_map_entities) {
        auto existing = world_runtime_.findEntity(entity.entity_id);
        if (existing.is_ok()) {
            const auto* ent = existing.value();
            // If existing entity is not OnMap (e.g. InInventory), that's a P45 ownership conflict
            if (ent->location_kind != world_runtime::WorldEntityLocationKind::OnMap) {
                return Result<WorldRegionApplyPlan>::fail(
                    makeError(ErrorCode::rule_conflict, "runtime_conflict_entity_moved",
                              "Entity " + entity.entity_id + " has location_kind " +
                              world_runtime::toString(ent->location_kind) +
                              ", expected OnMap. Restore refused to avoid overwriting pickup/drop state."));
            }
            // If existing entity is OnMap but in a different region, that's a cross-region conflict
            if (ent->coord.has_value()) {
                auto ent_region = WorldRegionMath::coordToRegion(
                    ent->coord->x, ent->coord->y, snapshot.header.region_key.region_size);
                if (ent_region.rx != snapshot.header.region_key.rx ||
                    ent_region.ry != snapshot.header.region_key.ry) {
                    return Result<WorldRegionApplyPlan>::fail(
                        makeError(ErrorCode::rule_conflict, "runtime_conflict_cross_region_entity",
                                  "Entity " + entity.entity_id + " already exists OnMap in a different region. "
                                  "Restore refused to avoid duplication."));
                }
            }
        }
    }

    plan.cells_to_apply = snapshot.cells;
    plan.entities_to_apply = snapshot.on_map_entities;
    plan.resource_nodes_to_apply = snapshot.resource_nodes;
    plan.exploration_slices_to_apply = snapshot.exploration_slices;

    return Result<WorldRegionApplyPlan>::ok(std::move(plan));
}

Result<WorldRegionRestoreResult> WorldGridRuntimeApplyPort::applyRestore(
    const WorldRegionApplyPlan& plan) {

    WorldRegionRestoreResult result;
    result.region_key = plan.region_key;
    result.status = WorldRegionRestoreStatus::Restored;
    result.availability_source = WorldRegionAvailabilitySource::RestoredSnapshot;
    result.state_version_before = world_runtime_.stateVersion();

    // Safety guard: if region became active between plan and apply, refuse.
    if (world_runtime_.isRegionGenerated(plan.region_key.regionRuntimeId())) {
        result.status = WorldRegionRestoreStatus::RuntimeConflict;
        result.failure_reason_keys.push_back("region_became_active_between_plan_and_apply");
        return Result<WorldRegionRestoreResult>::ok(std::move(result));
    }

    // 1. Apply cells
    for (const auto& cell : plan.cells_to_apply) {
        auto res = world_runtime_.createOrUpdateGeneratedCell(
            cell.coord,
            cell.terrain_key,
            cell.region_id,
            cell.blocks_movement,
            cell.movement_cost,
            cell.tag_keys);
        if (res.is_error()) {
            result.status = WorldRegionRestoreStatus::ApplyFailed;
            result.failure_reason_keys.push_back("cell_apply_failed:" + cell.cell_id);
            for (const auto& err : res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
            return Result<WorldRegionRestoreResult>::ok(std::move(result));
        }
        result.changed_cell_ids.push_back(cell.cell_id);
    }

    // 5. Apply entities
    for (const auto& entity : plan.entities_to_apply) {
        auto res = location_port_.spawnEntityOnMap(
            entity.entity_id,
            entity.entity_key,
            entity.display_name_key,
            entity.coord,
            entity.quantity,
            entity.stack_key,
            entity.stackable,
            entity.state_keys,
            entity.numeric_states,
            entity.tag_keys);
        if (res.is_error()) {
            result.status = WorldRegionRestoreStatus::ApplyFailed;
            result.failure_reason_keys.push_back("entity_apply_failed:" + entity.entity_id);
            for (const auto& err : res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
            return Result<WorldRegionRestoreResult>::ok(std::move(result));
        }
        result.changed_entity_ids.push_back(entity.entity_id);
    }

    // 6. Apply resource nodes
    for (const auto& node : plan.resource_nodes_to_apply) {
        world_runtime::WorldResourceNodeRuntime node_runtime;
        node_runtime.node_id = node.node_id;
        node_runtime.resource_key = node.resource_key;
        node_runtime.coord = node.coord;
        node_runtime.node_kind_str = node.node_kind_str;
        node_runtime.node_state_str = node.node_state_str;
        node_runtime.remaining_charges = node.remaining_charges;
        node_runtime.max_charges = node.max_charges;
        node_runtime.required_action_key = node.required_action_key;
        node_runtime.required_tool_key = node.required_tool_key;
        node_runtime.output_entity_keys = node.output_entity_keys;
        node_runtime.tag_keys = node.tag_keys;

        auto res = world_runtime_.upsertGeneratedResourceNode(node_runtime);
        if (res.is_error()) {
            result.status = WorldRegionRestoreStatus::ApplyFailed;
            result.failure_reason_keys.push_back("node_apply_failed:" + node.node_id);
            for (const auto& err : res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
            return Result<WorldRegionRestoreResult>::ok(std::move(result));
        }
        result.changed_resource_node_ids.push_back(node.node_id);
    }

    // 7. Restore exploration slices after cells exist and before the region is marked active.
    for (const auto& slice : plan.exploration_slices_to_apply) {
        auto res = world_runtime_.applyExplorationVisibility(
            slice.actor_key,
            slice.cell_visibility_by_id);
        if (res.is_error()) {
            result.status = WorldRegionRestoreStatus::ApplyFailed;
            result.failure_reason_keys.push_back("exploration_apply_failed:" + slice.actor_key);
            for (const auto& err : res.errors()) {
                result.failure_reason_keys.push_back(err.message_key);
            }
            return Result<WorldRegionRestoreResult>::ok(std::move(result));
        }
    }

    // 8. Mark region as generated only after all writes succeed
    world_runtime_.markRegionGenerated(plan.region_key.regionRuntimeId());

    result.state_version_after = world_runtime_.stateVersion();
    result.warning_keys = plan.warning_keys;

    return Result<WorldRegionRestoreResult>::ok(std::move(result));
}

} // namespace pathfinder::world_region_state
