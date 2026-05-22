#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cassert>

namespace pathfinder::world_inventory {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;

namespace {

// Helper to build full projection patch including inventory patches
WorldProjectionPatchDto buildFullProjectionPatch(
    const std::vector<std::string>& changed_cell_ids,
    const std::vector<std::string>& changed_entity_ids,
    const std::vector<std::string>& changed_inventory_ids,
    const std::string& viewer_actor_key,
    IWorldRuntime& world_runtime,
    const IInventoryRuntime& inventory_runtime) {

    WorldProjectionPatchDto patch;
    uint64_t combined_version = std::max(world_runtime.stateVersion(), inventory_runtime.stateVersion());
    patch.new_projection_version = combined_version;
    patch.requires_full_refresh = false;

    // Cell patches
    WorldProjectionAdapter world_proj_adapter;
    auto cell_patches = world_proj_adapter.buildCellPatches(changed_cell_ids, viewer_actor_key, world_runtime);
    if (cell_patches.is_ok()) {
        patch.changed_cells = std::move(cell_patches.value());
    }

    // Entity patches
    auto entity_patches = world_proj_adapter.buildEntityPatches(changed_entity_ids, viewer_actor_key, world_runtime);
    if (entity_patches.is_ok()) {
        patch.changed_entities = std::move(entity_patches.value());
    }

    // Inventory patches
    InventoryProjectionAdapter inv_proj_adapter;
    auto inv_patches = inv_proj_adapter.buildInventoryPatches(changed_inventory_ids, viewer_actor_key, inventory_runtime);
    if (inv_patches.is_ok()) {
        patch.changed_inventories = std::move(inv_patches.value());
    }

    return patch;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PickupCommandHandler
// ---------------------------------------------------------------------------

class PickupCommandHandler : public IWorldCommandHandler {
public:
    PickupCommandHandler(IInventoryRuntime& inventory_runtime, IWorldRuntime& world_runtime)
        : inventory_runtime_(inventory_runtime), world_runtime_(world_runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Pickup;
    }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {

        WorldCommandExecutionResult result;

        // Validate target entity
        if (command.target.target_entity_id.empty()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("missing_target_entity");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        // Parse quantity from parameters
        int quantity = 1;
        auto qit = command.parameters.find("quantity");
        if (qit != command.parameters.end()) {
            try {
                quantity = std::stoi(qit->second);
            } catch (...) {
                quantity = 1;
            }
        }

        InventoryTransferRequest request;
        request.transfer_id = command.command_id;
        request.transfer_kind = InventoryTransferKind::PickupFromMap;
        request.actor_key = command.actor_key;
        request.entity_id = command.target.target_entity_id;
        request.quantity = quantity;

        auto transfer_res = inventory_runtime_.transfer(request);
        if (transfer_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("transfer_internal_error");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto transfer_result = transfer_res.value();
        if (!transfer_result.ok) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back(toString(transfer_result.failure_kind));

            WorldEventDto event;
            event.event_id = command.command_id + "_blocked";
            event.event_kind = "PickupBlocked";
            event.tick = context.currentTick();
            event.title_text = "拾取受阻";
            event.body_text = "无法拾取物品: " + toString(transfer_result.failure_kind);
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));

            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_pickup";
        event.event_kind = "ItemPickedUp";
        event.tick = context.currentTick();
        event.title_text = "拾取";
        event.body_text = "你拾取了物品。";
        event.actor_key = command.actor_key;
        event.target_entity_id = command.target.target_entity_id;
        result.events.push_back(std::move(event));

        // Build full projection patch
        result.projection_patch_override = buildFullProjectionPatch(
            transfer_result.changed_cell_ids,
            transfer_result.changed_entity_ids,
            transfer_result.changed_inventory_ids,
            command.actor_key,
            world_runtime_,
            inventory_runtime_);

        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    IInventoryRuntime& inventory_runtime_;
    IWorldRuntime& world_runtime_;
};

std::shared_ptr<IWorldCommandHandler> createPickupCommandHandler(
    IInventoryRuntime& inventory_runtime,
    IWorldRuntime& world_runtime) {
    return std::make_shared<PickupCommandHandler>(inventory_runtime, world_runtime);
}

// ---------------------------------------------------------------------------
// DropCommandHandler
// ---------------------------------------------------------------------------

class DropCommandHandler : public IWorldCommandHandler {
public:
    DropCommandHandler(IInventoryRuntime& inventory_runtime, IWorldRuntime& world_runtime)
        : inventory_runtime_(inventory_runtime), world_runtime_(world_runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Drop;
    }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {

        WorldCommandExecutionResult result;

        // Parse entity_id / entity_key / quantity from command target and parameters
        std::string entity_id = command.target.target_entity_id;
        std::string entity_key;
        int quantity = 1;

        // Fallback to parameters for legacy / direct-command paths
        if (entity_id.empty()) {
            auto eid_it = command.parameters.find("entity_id");
            if (eid_it != command.parameters.end()) {
                entity_id = eid_it->second;
            }
        }
        auto ekey_it = command.parameters.find("entity_key");
        if (ekey_it != command.parameters.end()) {
            entity_key = ekey_it->second;
        }
        auto qit = command.parameters.find("quantity");
        if (qit != command.parameters.end()) {
            try {
                quantity = std::stoi(qit->second);
            } catch (...) {
                quantity = 1;
            }
        }

        if (entity_id.empty() && entity_key.empty()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("missing_item_ref");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        InventoryTransferRequest request;
        request.transfer_id = command.command_id;
        request.transfer_kind = InventoryTransferKind::DropToMap;
        request.actor_key = command.actor_key;
        request.entity_id = entity_id;
        request.entity_key = entity_key;
        request.quantity = quantity;
        request.parameters = command.parameters;

        auto transfer_res = inventory_runtime_.transfer(request);
        if (transfer_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("transfer_internal_error");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto transfer_result = transfer_res.value();
        if (!transfer_result.ok) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back(toString(transfer_result.failure_kind));

            WorldEventDto event;
            event.event_id = command.command_id + "_blocked";
            event.event_kind = "DropBlocked";
            event.tick = context.currentTick();
            event.title_text = "丢弃受阻";
            event.body_text = "无法丢弃物品: " + toString(transfer_result.failure_kind);
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));

            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;

        WorldEventDto event;
        event.event_id = command.command_id + "_drop";
        event.event_kind = "ItemDropped";
        event.tick = context.currentTick();
        event.title_text = "丢弃";
        event.body_text = "你丢弃了物品。";
        event.actor_key = command.actor_key;
        result.events.push_back(std::move(event));

        // Build full projection patch
        result.projection_patch_override = buildFullProjectionPatch(
            transfer_result.changed_cell_ids,
            transfer_result.changed_entity_ids,
            transfer_result.changed_inventory_ids,
            command.actor_key,
            world_runtime_,
            inventory_runtime_);

        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    IInventoryRuntime& inventory_runtime_;
    IWorldRuntime& world_runtime_;
};

std::shared_ptr<IWorldCommandHandler> createDropCommandHandler(
    IInventoryRuntime& inventory_runtime,
    IWorldRuntime& world_runtime) {
    return std::make_shared<DropCommandHandler>(inventory_runtime, world_runtime);
}

} // namespace pathfinder::world_inventory
