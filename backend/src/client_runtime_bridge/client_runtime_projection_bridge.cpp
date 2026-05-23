#include "pathfinder/client_runtime_bridge/client_runtime_projection_bridge.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_runtime::IWorldRuntime;
using pathfinder::world_runtime::WorldGridRuntime;
using pathfinder::world_runtime::WorldCellVisibility;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::InventoryPatchDto;
using pathfinder::world_command::PatchOp;

ClientRuntimeProjectionBridge::ClientRuntimeProjectionBridge(
    IWorldRuntime& runtime,
    ClientRuntimeBridgeMode mode)
    : ClientRuntimeProjectionBridge(runtime, nullptr, mode) {}

ClientRuntimeProjectionBridge::ClientRuntimeProjectionBridge(
    IWorldRuntime& runtime,
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime,
    ClientRuntimeBridgeMode mode)
    : runtime_(runtime)
    , inventory_runtime_(inventory_runtime)
    , mode_(mode) {}

Result<ClientRuntimeView> ClientRuntimeProjectionBridge::buildRuntimeView(
    const ClientRuntimeViewRequest& request) const {

    auto valid = request.validateBasic();
    if (valid.is_error()) {
        return Result<ClientRuntimeView>::fail(valid.errors());
    }

    if (mode_ != ClientRuntimeBridgeMode::RuntimeBacked) {
        return Result<ClientRuntimeView>::fail(
            makeError(ErrorCode::common_not_implemented, "bridge_mode_invalid", "Projection bridge not in RuntimeBacked mode"));
    }

    // Find actor
    auto actor_res = runtime_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<ClientRuntimeView>::fail(
            makeError(ErrorCode::id_not_found, "actor_not_found", "Actor not found: " + request.actor_key));
    }

    ClientRuntimeView view;
    view.projection_version = runtime_.stateVersion();
    view.actor_key = request.actor_key;
    view.active_layer_key = request.layer_key;

    auto cells_res = buildVisibleCells(request.actor_key, request.layer_key);
    if (cells_res.is_ok()) {
        view.visible_cells = std::move(cells_res.value());
    }

    auto entities_res = buildVisibleEntities(request.actor_key, request.layer_key);
    if (entities_res.is_ok()) {
        view.visible_entities = std::move(entities_res.value());
    }

    auto inventories_res = buildVisibleInventories(request.actor_key);
    if (inventories_res.is_ok()) {
        view.inventories = std::move(inventories_res.value());
    }

    return Result<ClientRuntimeView>::ok(std::move(view));
}

Result<std::vector<pathfinder::world_command::WorldCommandOptionDto>> ClientRuntimeProjectionBridge::buildRuntimeOptions(
    const ClientRuntimeCommandOptionRequest& /*request*/) const {
    // Projection bridge does not generate options.
    return Result<std::vector<pathfinder::world_command::WorldCommandOptionDto>>::ok(
        std::vector<pathfinder::world_command::WorldCommandOptionDto>{});
}

Result<ClientRuntimeBridgeDiagnostics> ClientRuntimeProjectionBridge::diagnostics(
    const std::string& actor_key,
    const std::string& layer_key) const {

    ClientRuntimeBridgeDiagnostics diag;
    diag.bridge_mode = mode_;
    diag.runtime_state_version = runtime_.stateVersion();

    auto cells_res = buildVisibleCells(actor_key, layer_key);
    if (cells_res.is_ok()) {
        diag.visible_cell_count = static_cast<int>(cells_res.value().size());
    }

    auto entities_res = buildVisibleEntities(actor_key, layer_key);
    if (entities_res.is_ok()) {
        diag.visible_entity_count = static_cast<int>(entities_res.value().size());
    }

    return Result<ClientRuntimeBridgeDiagnostics>::ok(std::move(diag));
}

Result<std::vector<WorldCellPatchDto>> ClientRuntimeProjectionBridge::buildVisibleCells(
    const std::string& actor_key,
    const std::string& /*layer_key*/) const {

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime_);
    if (!grid) {
        return Result<std::vector<WorldCellPatchDto>>::ok(std::vector<WorldCellPatchDto>{});
    }

    // TODO(P57): Replace snapshotForDebug with IWorldRuntimeProjectionQueryPort.
    // Current transition uses snapshotForDebug for compatibility; must upgrade before P58.
    auto snap_res = runtime_.snapshotForDebug();
    if (snap_res.is_error()) {
        return Result<std::vector<WorldCellPatchDto>>::ok(std::vector<WorldCellPatchDto>{});
    }

    const auto& snapshot = snap_res.value();
    std::vector<std::string> known_cell_ids;

    for (const auto& [cell_id, cell] : snapshot.cells) {
        auto visibility = grid->getCellVisibility(actor_key, cell_id);
        if (visibility != WorldCellVisibility::Unknown) {
            known_cell_ids.push_back(cell_id);
        }
    }

    auto patches = projection_adapter_.buildCellPatches(known_cell_ids, actor_key, runtime_);
    if (patches.is_ok()) {
        return Result<std::vector<WorldCellPatchDto>>::ok(std::move(patches.value()));
    }
    return Result<std::vector<WorldCellPatchDto>>::ok(std::vector<WorldCellPatchDto>{});
}

Result<std::vector<WorldEntityPatchDto>> ClientRuntimeProjectionBridge::buildVisibleEntities(
    const std::string& actor_key,
    const std::string& /*layer_key*/) const {

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime_);
    if (!grid) {
        return Result<std::vector<WorldEntityPatchDto>>::ok(std::vector<WorldEntityPatchDto>{});
    }

    // TODO(P57): Replace snapshotForDebug with IWorldRuntimeProjectionQueryPort.
    auto snap_res = runtime_.snapshotForDebug();
    if (snap_res.is_error()) {
        return Result<std::vector<WorldEntityPatchDto>>::ok(std::vector<WorldEntityPatchDto>{});
    }

    const auto& snapshot = snap_res.value();
    std::vector<std::string> visible_entity_ids;

    for (const auto& [entity_id, entity] : snapshot.entities) {
        if (!entity.coord) continue;
        auto visibility = grid->getCellVisibility(actor_key, entity.coord->cellId());
        if (visibility == WorldCellVisibility::Visible) {
            visible_entity_ids.push_back(entity_id);
        }
    }

    std::vector<WorldEntityPatchDto> result;
    auto patches = projection_adapter_.buildEntityPatches(visible_entity_ids, actor_key, runtime_);
    if (patches.is_ok()) {
        result = std::move(patches.value());
    }

    for (const auto& [node_id, node] : snapshot.resource_nodes) {
        if (node.coord.layer_key.empty()) continue;
        if (node.remaining_charges <= 0 || node.node_state_str == "Depleted") continue;
        auto visibility = grid->getCellVisibility(actor_key, node.coord.cellId());
        if (visibility != WorldCellVisibility::Visible) continue;

        WorldEntityPatchDto patch;
        patch.entity_id = node.node_id;
        patch.op = PatchOp::Update;
        patch.fields["entity_key"] = node.resource_key;
        patch.fields["display_name_key"] = "object." + node.resource_key + ".name";
        patch.fields["x"] = std::to_string(node.coord.x);
        patch.fields["y"] = std::to_string(node.coord.y);
        patch.fields["layer_key"] = node.coord.layer_key;
        patch.fields["resource_node"] = "true";
        patch.fields["remaining_charges"] = std::to_string(node.remaining_charges);
        result.push_back(std::move(patch));
    }

    return Result<std::vector<WorldEntityPatchDto>>::ok(std::move(result));
}

Result<std::vector<InventoryPatchDto>> ClientRuntimeProjectionBridge::buildVisibleInventories(
    const std::string& actor_key) const {

    if (!inventory_runtime_) {
        return Result<std::vector<InventoryPatchDto>>::ok(std::vector<InventoryPatchDto>{});
    }

    auto inventory_res = inventory_runtime_->findActorInventory(actor_key);
    if (inventory_res.is_error() || inventory_res.value() == nullptr) {
        return Result<std::vector<InventoryPatchDto>>::ok(std::vector<InventoryPatchDto>{});
    }

    return inventory_projection_adapter_.buildInventoryPatches(
        std::vector<std::string>{inventory_res.value()->inventory_id},
        actor_key,
        *inventory_runtime_);
}

} // namespace pathfinder::client_runtime_bridge
