#include "pathfinder/world_reaction/craft_command_handler.h"
#include "pathfinder/world_reaction/world_reaction_service.h"
#include "pathfinder/world_reaction/world_reaction_projection_bridge.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_inventory/iworld_inventory.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_reaction {

using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::foundation::ErrorCode;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::world_inventory;

// ---------------------------------------------------------------------------
// CraftCommandHandler
// ---------------------------------------------------------------------------

class CraftCommandHandler : public IWorldCommandHandler {
public:
    CraftCommandHandler(WorldReactionService& service,
                        IWorldRuntime& world_runtime,
                        IInventoryRuntime& inventory_runtime)
        : service_(service)
        , world_runtime_(world_runtime)
        , inventory_runtime_(inventory_runtime) {}

    WorldCommandKind kind() const override {
        return WorldCommandKind::Craft;
    }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {

        WorldCommandExecutionResult result;

        // Parse reaction_key from target.recipe_key or parameters
        std::string reaction_key;
        if (!command.target.recipe_key.empty()) {
            reaction_key = command.target.recipe_key;
        } else {
            auto it = command.parameters.find("reaction_key");
            if (it != command.parameters.end()) {
                reaction_key = it->second;
            }
        }

        if (reaction_key.empty()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("invalid_request");

            WorldEventDto event;
            event.event_id = command.command_id + "_failed";
            event.event_kind = "CraftFailed";
            event.tick = context.currentTick();
            event.title_text = "制作失败";
            event.body_text = "Missing reaction_key";
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        // Parse output_location_policy
        WorldReactionOutputLocationPolicy policy = WorldReactionOutputLocationPolicy::ActorInventory;
        auto policy_it = command.parameters.find("output_location_policy");
        if (policy_it != command.parameters.end()) {
            auto policy_res = worldReactionOutputLocationPolicyFromString(policy_it->second);
            if (policy_res.is_ok()) {
                policy = policy_res.value();
            } else {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back("output_policy_invalid");

                WorldEventDto event;
                event.event_id = command.command_id + "_failed";
                event.event_kind = "CraftFailed";
                event.tick = context.currentTick();
                event.title_text = "制作失败";
                event.body_text = "Invalid output_location_policy";
                event.actor_key = command.actor_key;
                result.events.push_back(std::move(event));
                return Result<WorldCommandExecutionResult>::ok(std::move(result));
            }
        }

        uint64_t base_version = std::max(world_runtime_.stateVersion(), inventory_runtime_.stateVersion());

        WorldReactionRequest req;
        req.reaction_command_id = command.command_id;
        req.actor_key = command.actor_key;
        req.reaction_key = reaction_key;
        req.action_kind = WorldReactionActionKind::Craft;
        req.output_location_policy = policy;
        req.parameters = command.parameters;

        auto reaction_result = service_.execute(req);
        if (reaction_result.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("reaction_internal_error");

            WorldEventDto event;
            event.event_id = command.command_id + "_failed";
            event.event_kind = "CraftFailed";
            event.tick = context.currentTick();
            event.title_text = "制作失败";
            event.body_text = "Internal reaction error";
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto& rr = reaction_result.value();
        if (!rr.ok) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back(toString(rr.failure_kind));

            WorldEventDto event;
            event.event_id = command.command_id + "_failed";
            event.event_kind = "CraftFailed";
            event.tick = context.currentTick();
            event.title_text = "制作失败";
            event.body_text = toString(rr.failure_kind);
            event.actor_key = command.actor_key;
            result.events.push_back(std::move(event));
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;

        // Forward state deltas
        for (const auto& delta : rr.state_deltas) {
            result.state_deltas.push_back(delta);
        }

        // Forward events with context tick
        for (auto event : rr.events) {
            event.tick = context.currentTick();
            result.events.push_back(std::move(event));
        }

        // Forward experiences with context tick
        for (auto exp : rr.experiences) {
            exp.tick = context.currentTick();
            result.experiences.push_back(std::move(exp));
        }

        // Forward changed ids
        for (const auto& id : rr.changed_cell_ids) {
            result.changed_cell_ids.push_back(id);
        }
        for (const auto& id : rr.changed_entity_ids) {
            result.changed_entity_ids.push_back(id);
        }

        // Build projection patch
        result.projection_patch_override = WorldReactionProjectionBridge::buildPatch(
            rr, command.actor_key, world_runtime_, inventory_runtime_, base_version);

        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    WorldReactionService& service_;
    IWorldRuntime& world_runtime_;
    IInventoryRuntime& inventory_runtime_;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

std::shared_ptr<IWorldCommandHandler> createCraftCommandHandler(
    WorldReactionService& service,
    IWorldRuntime& world_runtime,
    IInventoryRuntime& inventory_runtime) {
    return std::make_shared<CraftCommandHandler>(service, world_runtime, inventory_runtime);
}

} // namespace pathfinder::world_reaction
