#include "pathfinder/client_protocol/client_projection_adapter.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::client_protocol {

using pathfinder::foundation::Result;

// Test-only compatibility constructor. Production must use the bridge-injected
// constructor from ClientRuntimeHostFactory so projection data comes from runtime.
ClientProjectionAdapter::ClientProjectionAdapter() = default;

ClientProjectionAdapter::ClientProjectionAdapter(
    std::shared_ptr<pathfinder::client_runtime_bridge::IClientRuntimeBridgePort> bridge_port)
    : bridge_port_(std::move(bridge_port)) {}

Result<ClientWorldProjection> ClientProjectionAdapter::buildFromBridge(
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t projection_version,
    pathfinder::client_runtime_bridge::ClientProjectionBuildReason reason) const {

    pathfinder::client_runtime_bridge::ClientRuntimeViewRequest req;
    req.actor_key = actor_key;
    req.layer_key = layer_key.empty() ? "surface" : layer_key;
    req.projection_version_hint = projection_version;
    req.reason = reason;

    auto view_res = bridge_port_->buildRuntimeView(req);
    if (view_res.is_error()) {
        return Result<ClientWorldProjection>::fail(view_res.errors());
    }

    const auto& view = view_res.value();
    ClientWorldProjection proj;
    proj.projection_version = view.projection_version;
    proj.actor_key = view.actor_key;
    proj.active_layer_key = view.active_layer_key;
    proj.visible_cells = std::move(view.visible_cells);
    proj.visible_entities = std::move(view.visible_entities);
    proj.inventories = std::move(view.inventories);
    proj.knowledge = std::move(view.knowledge);
    proj.area_effects = std::move(view.area_effects);
    proj.safe_summary_keys = std::move(view.safe_summary_keys);
    // warning_keys from bridge are not exposed to client projection directly

    return Result<ClientWorldProjection>::ok(std::move(proj));
}

Result<ClientWorldProjection> ClientProjectionAdapter::buildFullProjection(
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t projection_version) const {

    if (bridge_port_) {
        return buildFromBridge(actor_key, layer_key, projection_version,
            pathfinder::client_runtime_bridge::ClientProjectionBuildReason::Bootstrap);
    }

    // P53 test fallback only: visible cells/entities are intentionally empty.
    // Production must never rely on this branch; ClientRuntimeHostFactory injects
    // ClientRuntimeProjectionBridge so the branch above returns real runtime data.
    ClientWorldProjection proj;
    proj.projection_version = projection_version;
    proj.actor_key = actor_key;
    proj.active_layer_key = layer_key.empty() ? "surface" : layer_key;
    return Result<ClientWorldProjection>::ok(std::move(proj));
}

Result<ClientWorldProjection> ClientProjectionAdapter::buildScopedProjection(
    const std::string& actor_key,
    const std::string& layer_key,
    uint64_t projection_version,
    const std::vector<ClientProjectionScope>& /*scopes*/) const {

    // P56: scopes are advisory; for now return full safe projection via bridge if available.
    // If no bridge exists, this intentionally falls back to the test-only full
    // projection stub above; do not use no-bridge mode in production wiring.
    if (bridge_port_) {
        return buildFromBridge(actor_key, layer_key, projection_version,
            pathfinder::client_runtime_bridge::ClientProjectionBuildReason::Refresh);
    }
    return buildFullProjection(actor_key, layer_key, projection_version);
}

Result<ClientWorldProjection> ClientProjectionAdapter::mergePatch(
    const ClientWorldProjection& base,
    const WorldProjectionPatchDto& patch) const {

    ClientWorldProjection merged = base;
    merged.projection_version = patch.new_projection_version;

    for (const auto& cell : patch.changed_cells) {
        if (cell.op == PatchOp::Remove || cell.op == PatchOp::Clear) {
            merged.visible_cells.erase(
                std::remove_if(merged.visible_cells.begin(), merged.visible_cells.end(),
                    [&](const auto& existing) { return existing.cell_id == cell.cell_id; }),
                merged.visible_cells.end());
            continue;
        }
        bool found = false;
        for (auto& existing : merged.visible_cells) {
            if (existing.cell_id == cell.cell_id) {
                existing = cell;
                found = true;
                break;
            }
        }
        if (!found) {
            merged.visible_cells.push_back(cell);
        }
    }

    for (const auto& entity : patch.changed_entities) {
        if (entity.op == PatchOp::Remove || entity.op == PatchOp::Clear) {
            merged.visible_entities.erase(
                std::remove_if(merged.visible_entities.begin(), merged.visible_entities.end(),
                    [&](const auto& existing) { return existing.entity_id == entity.entity_id; }),
                merged.visible_entities.end());
            continue;
        }
        bool found = false;
        for (auto& existing : merged.visible_entities) {
            if (existing.entity_id == entity.entity_id) {
                existing = entity;
                found = true;
                break;
            }
        }
        if (!found) {
            merged.visible_entities.push_back(entity);
        }
    }

    for (const auto& inv : patch.changed_inventories) {
        if (inv.op == PatchOp::Remove || inv.op == PatchOp::Clear) {
            merged.inventories.erase(
                std::remove_if(merged.inventories.begin(), merged.inventories.end(),
                    [&](const auto& existing) { return existing.inventory_id == inv.inventory_id; }),
                merged.inventories.end());
            continue;
        }
        bool found = false;
        for (auto& existing : merged.inventories) {
            if (existing.inventory_id == inv.inventory_id) {
                existing = inv;
                found = true;
                break;
            }
        }
        if (!found) {
            merged.inventories.push_back(inv);
        }
    }

    for (const auto& kn : patch.changed_knowledge) {
        if (kn.op == PatchOp::Remove || kn.op == PatchOp::Clear) {
            merged.knowledge.erase(
                std::remove_if(merged.knowledge.begin(), merged.knowledge.end(),
                    [&](const auto& existing) { return existing.actor_key == kn.actor_key; }),
                merged.knowledge.end());
            continue;
        }
        bool found = false;
        for (auto& existing : merged.knowledge) {
            if (existing.actor_key == kn.actor_key) {
                existing = kn;
                found = true;
                break;
            }
        }
        if (!found) {
            merged.knowledge.push_back(kn);
        }
    }

    for (const auto& ae : patch.changed_area_effects) {
        if (ae.op == PatchOp::Remove || ae.op == PatchOp::Clear) {
            merged.area_effects.erase(
                std::remove_if(merged.area_effects.begin(), merged.area_effects.end(),
                    [&](const auto& existing) { return existing.area_effect_id == ae.area_effect_id; }),
                merged.area_effects.end());
            continue;
        }
        bool found = false;
        for (auto& existing : merged.area_effects) {
            if (existing.area_effect_id == ae.area_effect_id) {
                existing = ae;
                found = true;
                break;
            }
        }
        if (!found) {
            merged.area_effects.push_back(ae);
        }
    }

    return Result<ClientWorldProjection>::ok(std::move(merged));
}

} // namespace pathfinder::client_protocol
