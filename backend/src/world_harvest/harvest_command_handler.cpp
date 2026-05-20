#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_harvest/resource_harvest_service.h"
#include "pathfinder/world_harvest/harvest_projection_bridge.h"

namespace pathfinder::world_command {

using pathfinder::foundation::Result;
using pathfinder::world_harvest::ResourceHarvestService;
using pathfinder::world_harvest::ResourceHarvestRequest;
using pathfinder::world_harvest::ResourceHarvestKind;
using pathfinder::world_harvest::HarvestProjectionBridge;
using pathfinder::world_harvest::ResourceHarvestFailureKind;

// ---------------------------------------------------------------------------
// Generic harvest command handler
// ---------------------------------------------------------------------------

class HarvestCommandHandler : public IWorldCommandHandler {
public:
    HarvestCommandHandler(ResourceHarvestKind kind, ResourceHarvestService& service)
        : kind_(kind), service_(service) {}

    WorldCommandKind kind() const override {
        switch (kind_) {
            case ResourceHarvestKind::Gather: return WorldCommandKind::Gather;
            case ResourceHarvestKind::Chop:   return WorldCommandKind::Chop;
            case ResourceHarvestKind::Mine:   return WorldCommandKind::Mine;
            case ResourceHarvestKind::Dig:    return WorldCommandKind::Dig;
            default: return WorldCommandKind::Unknown;
        }
    }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {

        WorldCommandExecutionResult result;

        // Build harvest request
        ResourceHarvestRequest request;
        request.harvest_id = command.command_id;
        request.actor_key = command.actor_key;
        request.harvest_kind = kind_;

        // node_id from target or parameters
        if (!command.target.target_entity_id.empty()) {
            request.node_id = command.target.target_entity_id;
        } else {
            auto it = command.parameters.find("node_id");
            if (it != command.parameters.end()) {
                request.node_id = it->second;
            }
        }

        // output location policy (P47 only supports DropOnMap)
        auto it_policy = command.parameters.find("output_location_policy");
        if (it_policy != command.parameters.end()) {
            if (it_policy->second == "prefer_inventory") {
                request.output_location_policy = pathfinder::world_harvest::HarvestOutputLocationPolicy::PreferInventory;
            } else if (it_policy->second == "drop_on_map") {
                request.output_location_policy = pathfinder::world_harvest::HarvestOutputLocationPolicy::DropOnMap;
            } else {
                request.output_location_policy = pathfinder::world_harvest::HarvestOutputLocationPolicy::Unknown;
            }
        } else {
            request.output_location_policy = pathfinder::world_harvest::HarvestOutputLocationPolicy::DropOnMap;
        }

        request.parameters = command.parameters;

        auto harvest_result = service_.execute(request);

        if (!harvest_result.ok) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back(
                pathfinder::world_harvest::toString(harvest_result.failure_kind));

            WorldEventDto event;
            event.event_id = command.command_id + "_failed";
            event.event_kind = "HarvestFailed";
            event.tick = context.currentTick();
            event.title_text = "采集失败";
            event.body_text = pathfinder::world_harvest::toString(harvest_result.failure_kind);
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));

            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;

        // Forward state deltas and events from harvest result
        for (const auto& delta : harvest_result.state_deltas) {
            result.state_deltas.push_back(delta);
        }
        for (auto event : harvest_result.events) {
            event.tick = context.currentTick();
            result.events.push_back(std::move(event));
        }

        // Changed IDs
        for (const auto& cell_id : harvest_result.changed_cell_ids) {
            result.changed_cell_ids.push_back(cell_id);
        }
        for (const auto& entity_id : harvest_result.changed_entity_ids) {
            result.changed_entity_ids.push_back(entity_id);
        }

        // Projection patch
        auto patch = HarvestProjectionBridge::buildPatch(
            harvest_result, context.currentTick());
        result.projection_patch_override = std::move(patch);

        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    ResourceHarvestKind kind_;
    ResourceHarvestService& service_;
};

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

std::shared_ptr<IWorldCommandHandler> createGatherCommandHandler(
    ResourceHarvestService& service) {
    return std::make_shared<HarvestCommandHandler>(ResourceHarvestKind::Gather, service);
}

std::shared_ptr<IWorldCommandHandler> createChopCommandHandler(
    ResourceHarvestService& service) {
    return std::make_shared<HarvestCommandHandler>(ResourceHarvestKind::Chop, service);
}

std::shared_ptr<IWorldCommandHandler> createMineCommandHandler(
    ResourceHarvestService& service) {
    return std::make_shared<HarvestCommandHandler>(ResourceHarvestKind::Mine, service);
}

std::shared_ptr<IWorldCommandHandler> createDigCommandHandler(
    ResourceHarvestService& service) {
    return std::make_shared<HarvestCommandHandler>(ResourceHarvestKind::Dig, service);
}

} // namespace pathfinder::world_command
