#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/content/content_registry.h"

#include "pathfinder/foundation/result.h"
#include "pathfinder/foundation/error.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::world_harvest {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::world_runtime::WorldResourceNodeRuntime;
using pathfinder::world_runtime::WorldActorRuntime;
using pathfinder::world_runtime::WorldCellCoord;
using pathfinder::world_inventory::InventoryOwnerRef;
using pathfinder::world_inventory::InventoryOwnerKind;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeOutputEntityId(const std::string& harvest_id,
                                      const std::string& entity_key,
                                      size_t index) {
    std::ostringstream oss;
    oss << harvest_id << "_" << entity_key << "_" << index;
    return oss.str();
}

static bool isAdjacentOrSame(const WorldCellCoord& a, const WorldCellCoord& b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return dx <= 1 && dy <= 1 && a.layer_key == b.layer_key;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ResourceHarvestService::ResourceHarvestService(
    world_runtime::IWorldRuntime& world_runtime,
    world_inventory::IWorldEntityLocationPort& location_port,
    world_inventory::IInventoryRuntime& inventory_runtime)
    : world_runtime_(world_runtime)
    , location_port_(location_port)
    , inventory_runtime_(inventory_runtime)
{
}

void ResourceHarvestService::setContentRegistry(std::shared_ptr<const pathfinder::content::ContentRegistry> registry) {
    content_registry_ = std::move(registry);
}

// ---------------------------------------------------------------------------
// Validate (read-only)
// ---------------------------------------------------------------------------

Result<ResourceHarvestDraft> ResourceHarvestService::validate(
    const ResourceHarvestRequest& request) const
{
    auto basic = request.validateBasic();
    if (basic.is_error()) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::command_invalid_argument,
                      toString(ResourceHarvestFailureKind::InvalidRequest),
                      basic.errors()[0].message_key));
    }

    // 1. Actor must exist
    auto actor_res = world_runtime_.findActor(request.actor_key);
    if (actor_res.is_error()) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::id_not_found,
                      toString(ResourceHarvestFailureKind::ActorMissing),
                      "actor not found"));
    }
    const auto* actor = actor_res.value();

    // 2. Node must exist
    auto node_res = world_runtime_.findResourceNode(request.node_id);
    if (node_res.is_error()) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::id_not_found,
                      toString(ResourceHarvestFailureKind::NodeMissing),
                      "resource node not found"));
    }
    const auto* node = node_res.value();

    // 3. Node not depleted
    if (node->remaining_charges <= 0) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::rule_conflict,
                      toString(ResourceHarvestFailureKind::NodeDepleted),
                      "resource node is depleted"));
    }
    if (node->node_state_str == "Depleted") {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::rule_conflict,
                      toString(ResourceHarvestFailureKind::NodeDepleted),
                      "resource node state is Depleted"));
    }

    // 4. Action kind matches node
    std::string expected_action = toString(request.harvest_kind);
    if (node->required_action_key != expected_action) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::validation_failed,
                      toString(ResourceHarvestFailureKind::ActionMismatch),
                      "harvest action not allowed on node"));
    }

    // 5. Actor must see the cell
    bool visible = location_port_.isCellVisibleToActor(
        request.actor_key, node->coord);
    if (!visible) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::precondition_target_unavailable,
                      toString(ResourceHarvestFailureKind::NodeNotVisible),
                      "cell not visible to actor"));
    }

    // 6. Distance check (adjacent or same cell)
    if (!isAdjacentOrSame(actor->coord, node->coord)) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::precondition_target_too_far,
                      toString(ResourceHarvestFailureKind::NodeTooFar),
                      "node too far from actor"));
    }

    // 7. Output location policy check (P47 MVP only supports DropOnMap)
    if (request.output_location_policy != HarvestOutputLocationPolicy::DropOnMap) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::command_invalid_argument,
                      toString(ResourceHarvestFailureKind::InvalidRequest),
                      "output_location_policy must be DropOnMap in P47"));
    }

    // 8. Output entity keys check
    if (node->output_entity_keys.empty()) {
        return Result<ResourceHarvestDraft>::fail(
            makeError(ErrorCode::validation_failed,
                      toString(ResourceHarvestFailureKind::OutputInvalid),
                      "node has no output_entity_keys"));
    }
    for (const auto& key : node->output_entity_keys) {
        if (key.empty()) {
            return Result<ResourceHarvestDraft>::fail(
                makeError(ErrorCode::validation_failed,
                          toString(ResourceHarvestFailureKind::OutputInvalid),
                          "output entity key is empty"));
        }
    }

    // 9. Required tool check
    if (!node->required_tool_key.empty()) {
        InventoryOwnerRef owner;
        owner.owner_kind = InventoryOwnerKind::Actor;
        owner.owner_key = request.actor_key;
        auto items_res = inventory_runtime_.queryItems(owner, node->required_tool_key);
        if (items_res.is_error()) {
            return Result<ResourceHarvestDraft>::fail(
                makeError(ErrorCode::precondition_missing_tool,
                          toString(ResourceHarvestFailureKind::ToolMissing),
                          "required tool query failed"));
        }
        auto items = items_res.value();
        bool has_tool = false;
        for (const auto& it : items) {
            if (it.entity_key == node->required_tool_key && it.quantity > 0) {
                has_tool = true;
                break;
            }
        }
        if (!has_tool) {
            return Result<ResourceHarvestDraft>::fail(
                makeError(ErrorCode::precondition_missing_tool,
                          toString(ResourceHarvestFailureKind::ToolMissing),
                          "required tool missing"));
        }
    }

    // 10. Build output drafts from object definitions when ContentRegistry is wired.
    std::vector<ResourceHarvestOutputDraft> outputs;
    for (size_t i = 0; i < node->output_entity_keys.size(); ++i) {
        const auto& entity_key = node->output_entity_keys[i];
        const pathfinder::content::ObjectDefinitionContent* object = nullptr;
        if (content_registry_) {
            object = content_registry_->findObject(entity_key);
            if (!object) {
                return Result<ResourceHarvestDraft>::fail(
                    makeError(ErrorCode::id_not_found,
                              toString(ResourceHarvestFailureKind::OutputInvalid),
                              "harvest output object is not registered: " + entity_key));
            }
        }

        ResourceHarvestOutputDraft draft;
        draft.entity_id = makeOutputEntityId(request.harvest_id, entity_key, i);
        draft.entity_key = entity_key;
        draft.display_name_key = object ? object->display_key : entity_key + "_name";
        draft.coord = node->coord;
        draft.quantity = 1;
        draft.stackable = true;
        draft.stack_key = entity_key + ":harvested";
        draft.state_keys = { "harvested" };
        draft.numeric_states = object ? object->default_numeric : std::map<std::string, double>{};
        draft.tag_keys = object ? object->safe_tags : std::vector<std::string>{};
        if (std::find(draft.tag_keys.begin(), draft.tag_keys.end(), "harvest_output") == draft.tag_keys.end()) {
            draft.tag_keys.push_back("harvest_output");
        }
        outputs.push_back(draft);
    }

    // 11. Build draft
    ResourceHarvestDraft draft;
    draft.draft_id = request.harvest_id;
    draft.request = request;
    draft.node_before = *node;
    draft.output_drafts = std::move(outputs);
    draft.charges_to_consume = 1;
    draft.will_deplete = (node->remaining_charges <= 1);
    draft.changed_cell_ids.push_back(node->coord.cellId());

    return Result<ResourceHarvestDraft>::ok(std::move(draft));
}

// ---------------------------------------------------------------------------
// Apply (write)
// ---------------------------------------------------------------------------

ResourceHarvestResult ResourceHarvestService::apply(const ResourceHarvestDraft& draft) {
    ResourceHarvestResult result;
    result.failure_kind = ResourceHarvestFailureKind::None;

    // Verify node still exists
    auto node_res = world_runtime_.findResourceNode(draft.request.node_id);
    if (node_res.is_error()) {
        result.failure_kind = ResourceHarvestFailureKind::RuntimeConflict;
        result.ok = false;
        return result;
    }
    const auto* node = node_res.value();

    // Verify node state hasn't changed (optimistic)
    if (node->remaining_charges != draft.node_before.remaining_charges) {
        result.failure_kind = ResourceHarvestFailureKind::RuntimeConflict;
        result.ok = false;
        return result;
    }

    // P47 atomicity note: current implementation spawns outputs before updating node.
    // This relies on the memory-backed runtime update being effectively infallible.
    // If future stages (P48+/P50+) introduce persistent storage or multi-runtime
    // coordination, this must be wrapped in a transaction or compensation mechanism.

    // 1. Spawn outputs on map (hard gate: only DropOnMap for P47)
    for (const auto& output : draft.output_drafts) {
        if (output.quantity <= 0) continue;

        auto spawn_res = location_port_.spawnEntityOnMap(
            output.entity_id,
            output.entity_key,
            output.display_name_key,
            output.coord,
            output.quantity,
            output.stack_key,
            output.stackable,
            output.state_keys,
            output.numeric_states,
            output.tag_keys
        );

        if (spawn_res.is_error()) {
            result.failure_kind = ResourceHarvestFailureKind::OutputSpawnFailed;
            result.ok = false;
            return result;
        }

        result.changed_entity_ids.push_back(output.entity_id);
    }

    // 2. Consume charge and update node
    WorldResourceNodeRuntime updated_node = *node;
    updated_node.remaining_charges -= draft.charges_to_consume;
    if (updated_node.remaining_charges < 0) updated_node.remaining_charges = 0;
    if (updated_node.remaining_charges == 0) {
        updated_node.node_state_str = "Depleted";
    }

    auto update_res = world_runtime_.updateResourceNodeRuntime(updated_node);
    if (update_res.is_error()) {
        result.failure_kind = ResourceHarvestFailureKind::RuntimeConflict;
        result.ok = false;
        return result;
    }

    result.changed_resource_node_ids.push_back(updated_node.node_id);
    result.changed_cell_ids.push_back(updated_node.coord.cellId());

    // 3. Build events
    {
        world_command::WorldEventDto event;
        event.event_id = draft.request.harvest_id + "_harvested";
        event.event_kind = "ResourceHarvested";
        event.tick = 0; // filled by caller if available
        event.title_text = "资源已采集";
        event.body_text = draft.request.actor_key + " 采集了 " + updated_node.resource_key;
        event.coord = pathfinder::world_command::WorldCoordinateDto{updated_node.coord.x, updated_node.coord.y, updated_node.coord.layer_key};
        event.actor_key = draft.request.actor_key;
        result.events.push_back(event);
    }

    // 4. Build state deltas
    {
        world_command::WorldStateDeltaDto delta;
        delta.delta_id = draft.request.harvest_id + "_node_update";
        delta.delta_kind = "ResourceNodeUpdated";
        delta.target_ref = draft.request.node_id;
        delta.op = world_command::PatchOp::Update;
        delta.fields = {
            { "remaining_charges", std::to_string(updated_node.remaining_charges) },
            { "previous_charges", std::to_string(draft.node_before.remaining_charges) },
            { "node_state", updated_node.node_state_str }
        };
        result.state_deltas.push_back(delta);
    }

    for (const auto& output : draft.output_drafts) {
        world_command::WorldStateDeltaDto delta;
        delta.delta_id = draft.request.harvest_id + "_output_" + output.entity_id;
        delta.delta_kind = "HarvestOutputCreated";
        delta.target_ref = output.entity_id;
        delta.op = world_command::PatchOp::Add;
        delta.fields = {
            { "entity_key", output.entity_key },
            { "coord", output.coord.cellId() },
            { "quantity", std::to_string(output.quantity) }
        };
        result.state_deltas.push_back(delta);
    }

    if (draft.will_deplete) {
        world_command::WorldEventDto event;
        event.event_id = draft.request.harvest_id + "_depleted";
        event.event_kind = "NodeDepleted";
        event.tick = 0;
        event.title_text = "资源耗尽";
        event.body_text = updated_node.resource_key + " 已耗尽";
        event.coord = pathfinder::world_command::WorldCoordinateDto{updated_node.coord.x, updated_node.coord.y, updated_node.coord.layer_key};
        event.actor_key = draft.request.actor_key;
        result.events.push_back(event);
    }

    result.ok = true;
    return result;
}

// ---------------------------------------------------------------------------
// Execute (validate + apply)
// ---------------------------------------------------------------------------

ResourceHarvestResult ResourceHarvestService::execute(const ResourceHarvestRequest& request) {
    auto draft_res = validate(request);
    if (draft_res.is_error()) {
        ResourceHarvestResult result;
        result.ok = false;
        std::string fk = draft_res.errors()[0].message_key;
        if (fk == toString(ResourceHarvestFailureKind::InvalidRequest))
            result.failure_kind = ResourceHarvestFailureKind::InvalidRequest;
        else if (fk == toString(ResourceHarvestFailureKind::ActorMissing))
            result.failure_kind = ResourceHarvestFailureKind::ActorMissing;
        else if (fk == toString(ResourceHarvestFailureKind::NodeMissing))
            result.failure_kind = ResourceHarvestFailureKind::NodeMissing;
        else if (fk == toString(ResourceHarvestFailureKind::NodeNotVisible))
            result.failure_kind = ResourceHarvestFailureKind::NodeNotVisible;
        else if (fk == toString(ResourceHarvestFailureKind::NodeTooFar))
            result.failure_kind = ResourceHarvestFailureKind::NodeTooFar;
        else if (fk == toString(ResourceHarvestFailureKind::NodeDepleted))
            result.failure_kind = ResourceHarvestFailureKind::NodeDepleted;
        else if (fk == toString(ResourceHarvestFailureKind::ActionMismatch))
            result.failure_kind = ResourceHarvestFailureKind::ActionMismatch;
        else if (fk == toString(ResourceHarvestFailureKind::ToolMissing))
            result.failure_kind = ResourceHarvestFailureKind::ToolMissing;
        else if (fk == toString(ResourceHarvestFailureKind::OutputInvalid))
            result.failure_kind = ResourceHarvestFailureKind::OutputInvalid;
        else if (fk == toString(ResourceHarvestFailureKind::RuntimeConflict))
            result.failure_kind = ResourceHarvestFailureKind::RuntimeConflict;
        else
            result.failure_kind = ResourceHarvestFailureKind::InvalidRequest;
        return result;
    }

    return apply(draft_res.value());
}

} // namespace pathfinder::world_harvest
