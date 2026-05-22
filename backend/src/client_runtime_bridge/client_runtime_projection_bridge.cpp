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
using pathfinder::world_command::PatchOp;

ClientRuntimeProjectionBridge::ClientRuntimeProjectionBridge(
    IWorldRuntime& runtime,
    ClientRuntimeBridgeMode mode)
    : runtime_(runtime)
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
    std::vector<std::string> visible_cell_ids;

    for (const auto& [cell_id, cell] : snapshot.cells) {
        auto visibility = grid->getCellVisibility(actor_key, cell_id);
        if (visibility == WorldCellVisibility::Visible) {
            visible_cell_ids.push_back(cell_id);
        }
    }

    auto patches = projection_adapter_.buildCellPatches(visible_cell_ids, actor_key, runtime_);
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

    auto patches = projection_adapter_.buildEntityPatches(visible_entity_ids, actor_key, runtime_);
    if (patches.is_ok()) {
        return Result<std::vector<WorldEntityPatchDto>>::ok(std::move(patches.value()));
    }
    return Result<std::vector<WorldEntityPatchDto>>::ok(std::vector<WorldEntityPatchDto>{});
}

} // namespace pathfinder::client_runtime_bridge
