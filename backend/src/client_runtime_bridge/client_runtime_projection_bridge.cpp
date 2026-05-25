#include "pathfinder/client_runtime_bridge/client_runtime_projection_bridge.h"
#include "pathfinder/world_runtime/world_grid_runtime.h"
#include "pathfinder/world_learning/world_knowledge_projection_bridge.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <utility>

namespace pathfinder::client_runtime_bridge {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::makeError;
using pathfinder::world_runtime::IWorldRuntime;
using pathfinder::world_runtime::WorldGridRuntime;
using pathfinder::world_runtime::WorldCellVisibility;
using pathfinder::world_runtime::WorldEntityLocationKind;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::InventoryPatchDto;
using pathfinder::world_command::KnowledgePatchDto;
using pathfinder::world_command::PatchOp;

namespace {

bool containsTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

std::string joinTags(const std::vector<std::string>& tags) {
    std::string result;
    for (const auto& tag : tags) {
        if (!result.empty()) result += ",";
        result += tag;
    }
    return result;
}

bool isSandboxCell(const pathfinder::world_runtime::WorldRuntimeSnapshot& snapshot,
                   const std::string& cell_id) {
    auto it = snapshot.cells.find(cell_id);
    return it != snapshot.cells.end() && containsTag(it->second.tag_keys, "sandbox");
}

std::string actorKeyForEntity(const pathfinder::world_runtime::WorldRuntimeSnapshot& snapshot,
                              const std::string& entity_id) {
    for (const auto& [actor_key, actor] : snapshot.actors) {
        if (actor.entity_id == entity_id) return actor_key;
    }
    return {};
}

} // namespace

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

    auto knowledge_res = buildVisibleKnowledge(request.actor_key);
    if (knowledge_res.is_ok()) {
        view.knowledge = std::move(knowledge_res.value());
    }

    return Result<ClientRuntimeView>::ok(std::move(view));
}

void ClientRuntimeProjectionBridge::setKnowledgeServices(
    const pathfinder::knowledge::KnowledgeRepository* knowledge_repository,
    std::shared_ptr<const pathfinder::content::ContentRegistry> content_registry) {
    knowledge_repository_ = knowledge_repository;
    content_registry_ = std::move(content_registry);
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
    std::vector<WorldCellPatchDto> sandbox_patches;
    std::vector<std::string> known_cell_ids;

    for (const auto& [cell_id, cell] : snapshot.cells) {
        if (containsTag(cell.tag_keys, "sandbox")) {
            WorldCellPatchDto patch;
            patch.cell_id = cell_id;
            patch.op = PatchOp::Update;
            patch.fields["x"] = std::to_string(cell.coord.x);
            patch.fields["y"] = std::to_string(cell.coord.y);
            patch.fields["layer_key"] = cell.coord.layer_key;
            patch.fields["terrain_key"] = cell.terrain_key;
            patch.fields["region_id"] = cell.region_id;
            patch.fields["visibility"] = "Visible";
            patch.fields["blocks_movement"] = cell.blocks_movement ? "true" : "false";
            patch.fields["movement_cost"] = std::to_string(cell.movement_cost);
            std::string entity_list;
            for (const auto& eid : cell.entity_ids) {
                if (!entity_list.empty()) entity_list += ",";
                entity_list += eid;
            }
            if (!entity_list.empty()) patch.fields["entity_ids"] = entity_list;
            const auto tags = joinTags(cell.tag_keys);
            if (!tags.empty()) patch.fields["tag_keys"] = tags;
            sandbox_patches.push_back(std::move(patch));
            continue;
        }

        auto visibility = grid->getCellVisibility(actor_key, cell_id);
        if (visibility != WorldCellVisibility::Unknown) {
            known_cell_ids.push_back(cell_id);
        }
    }

    auto patches = projection_adapter_.buildCellPatches(known_cell_ids, actor_key, runtime_);
    if (patches.is_ok()) {
        auto result = std::move(sandbox_patches);
        result.insert(result.end(), patches.value().begin(), patches.value().end());
        return Result<std::vector<WorldCellPatchDto>>::ok(std::move(result));
    }
    return Result<std::vector<WorldCellPatchDto>>::ok(std::move(sandbox_patches));
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
    std::vector<WorldEntityPatchDto> sandbox_entity_patches;
    std::vector<std::string> visible_entity_ids;

    for (const auto& [entity_id, entity] : snapshot.entities) {
        if (!entity.coord) continue;
        if (entity.location_kind == WorldEntityLocationKind::OnMap && isSandboxCell(snapshot, entity.coord->cellId())) {
            WorldEntityPatchDto patch;
            patch.entity_id = entity_id;
            patch.op = PatchOp::Update;
            patch.fields["entity_id"] = entity.entity_id;
            patch.fields["entity_key"] = entity.entity_key;
            patch.fields["display_name_key"] = entity.display_name_key;
            patch.fields["x"] = std::to_string(entity.coord->x);
            patch.fields["y"] = std::to_string(entity.coord->y);
            patch.fields["layer_key"] = entity.coord->layer_key;
            patch.fields["location_kind"] = toString(entity.location_kind);
            patch.fields["visible"] = entity.visible_by_default ? "true" : "false";
            patch.fields["blocks_movement"] = entity.blocks_movement ? "true" : "false";
            const auto entity_actor_key = actorKeyForEntity(snapshot, entity.entity_id);
            if (!entity_actor_key.empty()) patch.fields["actor_key"] = entity_actor_key;
            for (const auto& [key, value] : entity.numeric_states) {
                patch.fields["numeric_" + key] = std::to_string(value);
                if (key == "health" || key == "max_health") {
                    patch.fields[key] = std::to_string(static_cast<int>(value));
                } else if (key == "alive") {
                    patch.fields[key] = value > 0.0 ? "true" : "false";
                }
            }
            const auto tags = joinTags(entity.tag_keys);
            if (!tags.empty()) patch.fields["tag_keys"] = tags;
            sandbox_entity_patches.push_back(std::move(patch));
            continue;
        }
        auto visibility = grid->getCellVisibility(actor_key, entity.coord->cellId());
        if (visibility != WorldCellVisibility::Unknown) {
            visible_entity_ids.push_back(entity_id);
        }
    }

    std::vector<WorldEntityPatchDto> result;
    auto patches = projection_adapter_.buildEntityPatches(visible_entity_ids, actor_key, runtime_);
    if (patches.is_ok()) {
        result = std::move(patches.value());
    }
    result.insert(result.end(), sandbox_entity_patches.begin(), sandbox_entity_patches.end());

    for (const auto& [node_id, node] : snapshot.resource_nodes) {
        if (node.coord.layer_key.empty()) continue;
        if (node.remaining_charges <= 0 || node.node_state_str == "Depleted") continue;
        auto visibility = grid->getCellVisibility(actor_key, node.coord.cellId());
        if (visibility == WorldCellVisibility::Unknown && !isSandboxCell(snapshot, node.coord.cellId())) continue;

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

    std::vector<std::string> inventory_ids;
    auto add_actor_inventory = [&](const std::string& owner_actor_key) {
        if (owner_actor_key.empty()) return;
        auto inventory_res = inventory_runtime_->findActorInventory(owner_actor_key);
        if (inventory_res.is_error() || inventory_res.value() == nullptr) return;
        const auto& id = inventory_res.value()->inventory_id;
        if (std::find(inventory_ids.begin(), inventory_ids.end(), id) == inventory_ids.end()) {
            inventory_ids.push_back(id);
        }
    };

    add_actor_inventory(actor_key);

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime_);
    auto snap_res = runtime_.snapshotForDebug();
    if (grid && snap_res.is_ok()) {
        for (const auto& [candidate_actor_key, candidate_actor] : snap_res.value().actors) {
            if (candidate_actor_key == actor_key) continue;
            const auto visibility = grid->getCellVisibility(actor_key, candidate_actor.coord.cellId());
            if (visibility == WorldCellVisibility::Unknown && !isSandboxCell(snap_res.value(), candidate_actor.coord.cellId())) continue;
            add_actor_inventory(candidate_actor_key);
        }
    }

    if (inventory_ids.empty()) {
        return Result<std::vector<InventoryPatchDto>>::ok(std::vector<InventoryPatchDto>{});
    }

    return inventory_projection_adapter_.buildInventoryPatches(
        inventory_ids,
        actor_key,
        *inventory_runtime_);
}

Result<std::vector<KnowledgePatchDto>> ClientRuntimeProjectionBridge::buildVisibleKnowledge(
    const std::string& actor_key) const {

    if (!knowledge_repository_) {
        return Result<std::vector<KnowledgePatchDto>>::ok(std::vector<KnowledgePatchDto>{});
    }

    std::vector<std::string> visible_actor_keys;
    auto add_actor = [&](const std::string& candidate) {
        if (candidate.empty()) return;
        if (std::find(visible_actor_keys.begin(), visible_actor_keys.end(), candidate) == visible_actor_keys.end()) {
            visible_actor_keys.push_back(candidate);
        }
    };

    add_actor(actor_key);

    const WorldGridRuntime* grid = dynamic_cast<const WorldGridRuntime*>(&runtime_);
    auto snap_res = runtime_.snapshotForDebug();
    if (grid && snap_res.is_ok()) {
        const auto& snapshot = snap_res.value();
        for (const auto& [candidate_actor_key, candidate_actor] : snapshot.actors) {
            if (candidate_actor_key == actor_key) continue;
            const auto cell_id = candidate_actor.coord.cellId();
            const auto visibility = grid->getCellVisibility(actor_key, cell_id);
            if (visibility == WorldCellVisibility::Unknown && !isSandboxCell(snapshot, cell_id)) continue;
            add_actor(candidate_actor_key);
        }
    }

    pathfinder::world_learning::WorldKnowledgeProjectionBridge projector(content_registry_.get());
    std::vector<KnowledgePatchDto> patches;
    for (const auto& visible_actor_key : visible_actor_keys) {
        pathfinder::knowledge::KnowledgeOwner owner;
        owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
        owner.entity_id = pathfinder::foundation::EntityId(visible_actor_key);
        auto claims_res = knowledge_repository_->listByOwner(owner);
        if (claims_res.is_error() || claims_res.value().empty()) continue;
        auto projected = projector.project(claims_res.value(), {}, visible_actor_key, runtime_.stateVersion());
        patches.insert(
            patches.end(),
            projected.patch.changed_knowledge.begin(),
            projected.patch.changed_knowledge.end());
    }

    return Result<std::vector<KnowledgePatchDto>>::ok(std::move(patches));
}

} // namespace pathfinder::client_runtime_bridge
