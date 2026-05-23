#include "pathfinder/client_http/client_server_runtime_factory.h"
#include "pathfinder/client_http/client_http_gateway.h"
#include "pathfinder/world_command/world_command_handlers.h"
#include "pathfinder/world_runtime/world_command_runtime_handlers.h"
#include "pathfinder/world_generation/world_generation_service.h"
#include "pathfinder/world_generation/world_generation_applier.h"
#include "pathfinder/world_generation/world_generation_command_handler.h"
#include "pathfinder/world_generation/world_region_ensure_service.h"
#include "pathfinder/world_generation/move_target_region_guard.h"
#include "pathfinder/client_runtime_bridge/client_world_region_ensure_adapter.h"
#include "pathfinder/world_harvest/harvest_command_handler.h"
#include "pathfinder/world_reaction/craft_command_handler.h"
#include "pathfinder/world_inventory/world_inventory_command_handlers.h"
#include "pathfinder/world_map_interaction/region_activity_window_service.h"
#include "pathfinder/world_map_interaction/region_lifecycle_trigger_service.h"
#include "pathfinder/world_map_interaction/client_map_selection_service.h"
#include "pathfinder/world_teaching/teach_command_handler.h"
#include "pathfinder/world_teaching/iworld_actor_query_port.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include "pathfinder/world_harvest/harvest_projection_bridge.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/content/json_content_loader.h"
#include <iostream>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace pathfinder::client_http {

using namespace pathfinder::client_protocol;
using namespace pathfinder::world_command;
using namespace pathfinder::world_runtime;
using namespace pathfinder::client_runtime_bridge;

namespace {


pathfinder::knowledge::KnowledgeOwner actorKnowledgeOwner(const std::string& actor_key) {
    pathfinder::knowledge::KnowledgeOwner owner;
    owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    return owner;
}

int recipeInputQuantity(const pathfinder::content::ReactionInputDto& input) {
    return input.min > 0 ? input.min : (input.quantity > 0 ? input.quantity : 1);
}

std::string objectName(const pathfinder::content::ContentRegistry& registry, const std::string& object_key) {
    const auto key = "object." + object_key + ".name";
    auto translated = registry.translate("zh_cn", key);
    return translated == key ? object_key : translated;
}

std::string joinStrings(const std::vector<std::string>& values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    return oss.str();
}

std::string numericStatesParam(const std::map<std::string, double>& values) {
    std::ostringstream oss;
    size_t index = 0;
    for (const auto& [key, value] : values) {
        if (index++ > 0) oss << ",";
        oss << key << "=" << value;
    }
    return oss.str();
}

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool hasState(const std::vector<std::string>& states, const std::string& state) {
    return state.empty() || std::find(states.begin(), states.end(), state) != states.end();
}

int manhattan(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b) {
    if (a.layer_key != b.layer_key) return 1000000;
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool sameOrAdjacent8(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b) {
    return a.layer_key == b.layer_key && std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;
}

std::optional<pathfinder::world_runtime::WorldCellCoord> nearestOpenCoord(
    const pathfinder::world_runtime::IWorldRuntime& runtime,
    const pathfinder::world_runtime::WorldCellCoord& preferred,
    int radius = 6) {
    for (int r = 0; r <= radius; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) + std::abs(dy) != r) continue;
                pathfinder::world_runtime::WorldCellCoord coord{preferred.x + dx, preferred.y + dy, preferred.layer_key};
                auto cell_res = runtime.findCell(coord);
                if (cell_res.is_error()) continue;
                if (cell_res.value()->blocks_movement) continue;
                return coord;
            }
        }
    }
    return std::nullopt;
}

class RuntimeActorQueryPort final : public pathfinder::world_teaching::IWorldActorQueryPort {
public:
    explicit RuntimeActorQueryPort(pathfinder::world_runtime::IWorldRuntime& runtime) : runtime_(runtime) {}
    std::optional<pathfinder::world_runtime::WorldActorRuntime> findActor(const std::string& actor_key) const override {
        auto res = runtime_.findActor(actor_key);
        if (res.is_error()) return std::nullopt;
        return *res.value();
    }
private:
    pathfinder::world_runtime::IWorldRuntime& runtime_;
};


class NpcKnowledgeInspectCommandHandler final : public pathfinder::world_command::IWorldCommandHandler {
public:
    NpcKnowledgeInspectCommandHandler(
        pathfinder::world_runtime::IWorldRuntime& runtime,
        pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
        const pathfinder::content::ContentRegistry& content_registry)
        : bridge_(runtime)
        , knowledge_repository_(knowledge_repository)
        , content_registry_(content_registry) {}

    pathfinder::world_command::WorldCommandKind kind() const override {
        return pathfinder::world_command::WorldCommandKind::Inspect;
    }

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override {
        if (command.command_key != "inspect_actor_knowledge") {
            return bridge_.handleInspect(context, command);
        }

        pathfinder::world_command::WorldCommandExecutionResult result;
        const auto actor_key = command.target.target_actor_key;
        if (actor_key.empty()) {
            result.result_kind = pathfinder::world_command::WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("inspect_actor_missing");
            return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto claims_res = knowledge_repository_.listByOwner(actorKnowledgeOwner(actor_key));
        if (claims_res.is_error()) {
            result.result_kind = pathfinder::world_command::WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("inspect_actor_knowledge_failed");
            return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
        }

        std::ostringstream body;
        const auto& claims = claims_res.value();
        if (claims.empty()) {
            body << "这个NPC还没有可用知识。";
        } else {
            body << "这个NPC知道：";
            for (size_t index = 0; index < claims.size(); ++index) {
                const auto& claim = claims[index];
                if (index > 0) body << "；";
                body << readableClaim(claim);
            }
        }

        pathfinder::world_command::WorldEventDto event;
        event.event_id = command.command_id + "_npc_knowledge";
        event.event_kind = "NpcKnowledgeInspected";
        event.tick = context.currentTick();
        event.title_text = "NPC知识";
        event.body_text = body.str();
        event.actor_key = command.actor_key;
        event.target_entity_id = command.target.target_entity_id;
        result.result_kind = pathfinder::world_command::WorldCommandResultKind::Succeeded;
        result.events.push_back(std::move(event));
        return pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    mutable pathfinder::world_runtime::WorldCommandRuntimeBridge bridge_;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository_;
    const pathfinder::content::ContentRegistry& content_registry_;

    std::string readableClaim(const pathfinder::knowledge::KnowledgeClaim& claim) const {
        for (const auto* tmpl : content_registry_.allKnowledgeTemplates()) {
            if (!tmpl) continue;
            if (tmpl->subject_object_key == claim.subject.subject_id &&
                tmpl->action_key == claim.predicate.action_key &&
                tmpl->effect_key == claim.predicate.effect_key &&
                !tmpl->summary_key.empty()) {
                const auto translated = content_registry_.translate("zh_cn", tmpl->summary_key);
                if (translated != tmpl->summary_key) return translated;
            }
        }
        return objectName(content_registry_, claim.subject.subject_id) + " " + claim.predicate.action_key + " -> " + claim.predicate.effect_key;
    }
};

class NpcWorkUseCommandHandler final : public pathfinder::world_command::IWorldCommandHandler {
public:
    NpcWorkUseCommandHandler(
        pathfinder::world_runtime::WorldGridRuntime& runtime,
        pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
        pathfinder::world_harvest::ResourceHarvestService& harvest_service,
        pathfinder::world_reaction::WorldReactionService& reaction_service,
        const pathfinder::content::ContentRegistry& content_registry,
        pathfinder::knowledge::KnowledgeRepository& knowledge_repository)
        : runtime_(runtime)
        , inventory_runtime_(inventory_runtime)
        , harvest_service_(harvest_service)
        , reaction_service_(reaction_service)
        , content_registry_(content_registry)
        , knowledge_repository_(knowledge_repository) {}

    pathfinder::world_command::WorldCommandKind kind() const override {
        return pathfinder::world_command::WorldCommandKind::Use;
    }

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override {

        using namespace pathfinder::world_command;
        WorldCommandExecutionResult result;
        if (command.command_key != "order_actor_craft") {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("unsupported_use_command");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const std::string worker_actor_key = command.target.target_actor_key;
        const std::string reaction_key = command.target.recipe_key;
        if (worker_actor_key.empty() || reaction_key.empty()) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_missing_target_or_recipe");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const auto* reaction = content_registry_.findReaction(reaction_key);
        if (!reaction) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_reaction_missing");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        if (!workerKnowsRecipe(worker_actor_key, *reaction)) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_knowledge_missing");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        std::set<std::string> changed_cells;
        std::set<std::string> changed_entities;
        std::set<std::string> changed_inventories;

        auto ensure_inv = inventory_runtime_.ensureActorInventory(worker_actor_key);
        if (ensure_inv.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("npc_inventory_missing");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }
        changed_inventories.insert(ensure_inv.value());

        for (const auto& input : reaction->inputs) {
            const int required = recipeInputQuantity(input);
            if (!reactionConsumesInput(*reaction, input.object_key, input.state)) {
                if (inventoryQuantity(worker_actor_key, input.object_key, input.state) <= 0 &&
                    !moveNearMapInput(worker_actor_key, input, changed_cells, changed_entities)) {
                    result.result_kind = WorldCommandResultKind::Blocked;
                    result.failure_reason_keys.push_back("npc_work_missing_catalyst:" + input.object_key);
                    buildPatch(result, command.actor_key, changed_cells, changed_entities, changed_inventories, context.currentTick());
                    return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
                }
                continue;
            }

            int available = inventoryQuantity(worker_actor_key, input.object_key, input.state);
            int guard = 0;
            while (available < required && guard++ < 8) {
                if (!acquireInput(worker_actor_key, input, required - available, changed_cells, changed_entities, changed_inventories, result)) {
                    break;
                }
                available = inventoryQuantity(worker_actor_key, input.object_key, input.state);
            }
            if (available < required) {
                result.result_kind = WorldCommandResultKind::Blocked;
                result.failure_reason_keys.push_back("npc_work_missing_input:" + input.object_key);
                buildPatch(result, command.actor_key, changed_cells, changed_entities, changed_inventories, context.currentTick());
                return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
            }
        }

        pathfinder::world_reaction::WorldReactionRequest craft_req;
        craft_req.reaction_command_id = command.command_id + "_npc_craft";
        craft_req.actor_key = worker_actor_key;
        craft_req.reaction_key = reaction_key;
        craft_req.action_kind = pathfinder::world_reaction::WorldReactionActionKind::Craft;
        craft_req.output_location_policy = pathfinder::world_reaction::WorldReactionOutputLocationPolicy::ActorInventory;
        auto craft_res = reaction_service_.execute(craft_req);
        if (craft_res.is_error() || !craft_res.value().ok) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_craft_failed");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }
        auto craft = craft_res.value();
        for (const auto& id : craft.changed_cell_ids) changed_cells.insert(id);
        for (const auto& id : craft.changed_entity_ids) changed_entities.insert(id);
        for (const auto& id : craft.changed_inventory_ids) changed_inventories.insert(id);
        result.events.insert(result.events.end(), craft.events.begin(), craft.events.end());
        result.state_deltas.insert(result.state_deltas.end(), craft.state_deltas.begin(), craft.state_deltas.end());
        result.experiences.insert(result.experiences.end(), craft.experiences.begin(), craft.experiences.end());

        deliverOutputsToPlayer(command, *reaction, changed_entities, changed_inventories, result);
        maybeCounterVisibleThreat(worker_actor_key, *reaction, changed_entities, result, context.currentTick());

        WorldEventDto event;
        event.event_id = command.command_id + "_npc_work_done";
        event.event_kind = "NpcWorkOrderCompleted";
        event.tick = context.currentTick();
        event.title_text = "NPC完成代工";
        event.body_text = "NPC按已学知识完成了" + reaction_key + "，并把产物交给你。";
        event.actor_key = worker_actor_key;
        result.events.push_back(std::move(event));

        result.result_kind = WorldCommandResultKind::Succeeded;
        buildPatch(result, command.actor_key, changed_cells, changed_entities, changed_inventories, context.currentTick());
        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    pathfinder::world_runtime::WorldGridRuntime& runtime_;
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime_;
    pathfinder::world_harvest::ResourceHarvestService& harvest_service_;
    pathfinder::world_reaction::WorldReactionService& reaction_service_;
    const pathfinder::content::ContentRegistry& content_registry_;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository_;

    bool workerKnowsRecipe(const std::string& actor_key, const pathfinder::content::ReactionDefinitionContent& reaction) const {
        auto claims_res = knowledge_repository_.listByOwner(actorKnowledgeOwner(actor_key));
        if (claims_res.is_error()) return false;
        for (const auto& claim : claims_res.value()) {
            if (!claim.projection_flags.usable_by_ai) continue;
            if ((claim.predicate.action_key == reaction.action_key || claim.predicate.action_key == "craft") &&
                claim.predicate.effect_key == reaction.effect_key) return true;
        }
        return false;
    }

    bool reactionConsumesInput(const pathfinder::content::ReactionDefinitionContent& reaction,
                               const std::string& object_key,
                               const std::string& state) const {
        for (const auto& consume : reaction.consume) {
            if (consume.object_key != object_key) continue;
            if (!state.empty() && !consume.state.empty() && consume.state != state) continue;
            return true;
        }
        return false;
    }

    int inventoryQuantity(const std::string& actor_key, const std::string& object_key, const std::string& state) const {
        pathfinder::world_inventory::InventoryOwnerRef owner{pathfinder::world_inventory::InventoryOwnerKind::Actor, actor_key};
        auto items = inventory_runtime_.queryItems(owner, object_key);
        if (items.is_error()) return 0;
        int total = 0;
        for (const auto& item : items.value()) {
            if (hasState(item.state_keys, state)) total += item.quantity;
        }
        return total;
    }

    bool moveActorNear(const std::string& actor_key,
                       const pathfinder::world_runtime::WorldCellCoord& target,
                       std::set<std::string>& changed_cells,
                       std::set<std::string>& changed_entities) const {
        for (int step = 0; step < 48; ++step) {
            auto actor_res = runtime_.findActor(actor_key);
            if (actor_res.is_error()) return false;
            auto actor = *actor_res.value();
            if (sameOrAdjacent8(actor.coord, target)) return true;

            std::vector<pathfinder::world_runtime::WorldCellCoord> candidates;
            const int sx = (target.x > actor.coord.x) ? 1 : (target.x < actor.coord.x ? -1 : 0);
            const int sy = (target.y > actor.coord.y) ? 1 : (target.y < actor.coord.y ? -1 : 0);
            if (sx != 0) candidates.push_back({actor.coord.x + sx, actor.coord.y, actor.coord.layer_key});
            if (sy != 0) candidates.push_back({actor.coord.x, actor.coord.y + sy, actor.coord.layer_key});
            candidates.push_back({actor.coord.x + 1, actor.coord.y, actor.coord.layer_key});
            candidates.push_back({actor.coord.x - 1, actor.coord.y, actor.coord.layer_key});
            candidates.push_back({actor.coord.x, actor.coord.y + 1, actor.coord.layer_key});
            candidates.push_back({actor.coord.x, actor.coord.y - 1, actor.coord.layer_key});
            std::sort(candidates.begin(), candidates.end(), [&](const auto& left, const auto& right) {
                return manhattan(left, target) < manhattan(right, target);
            });
            bool moved = false;
            for (const auto& candidate : candidates) {
                auto move = runtime_.moveActor(actor_key, candidate);
                if (move.is_error() || !move.value().moved) continue;
                for (const auto& id : move.value().changed_cell_ids) changed_cells.insert(id);
                for (const auto& id : move.value().changed_entity_ids) changed_entities.insert(id);
                moved = true;
                break;
            }
            if (!moved) return false;
        }
        return false;
    }

    bool moveNearMapInput(const std::string& actor_key,
                          const pathfinder::content::ReactionInputDto& input,
                          std::set<std::string>& changed_cells,
                          std::set<std::string>& changed_entities) const {
        auto snap_res = runtime_.snapshotForDebug();
        if (snap_res.is_error()) return false;
        auto actor_res = runtime_.findActor(actor_key);
        if (actor_res.is_error()) return false;
        auto actor = *actor_res.value();

        std::vector<pathfinder::world_runtime::WorldEntityInstance> candidates;
        for (const auto& [entity_id, entity] : snap_res.value().entities) {
            if (entity.entity_key != input.object_key) continue;
            if (entity.location_kind != pathfinder::world_runtime::WorldEntityLocationKind::OnMap || !entity.coord) continue;
            if (!hasState(entity.state_keys, input.state)) continue;
            candidates.push_back(entity);
        }
        std::sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
            return manhattan(actor.coord, *a.coord) < manhattan(actor.coord, *b.coord);
        });
        for (const auto& entity : candidates) {
            if (moveActorNear(actor_key, *entity.coord, changed_cells, changed_entities)) return true;
        }
        return false;
    }

    bool acquireInput(const std::string& actor_key,
                      const pathfinder::content::ReactionInputDto& input,
                      int missing,
                      std::set<std::string>& changed_cells,
                      std::set<std::string>& changed_entities,
                      std::set<std::string>& changed_inventories,
                      pathfinder::world_command::WorldCommandExecutionResult& result) const {
        auto snap_res = runtime_.snapshotForDebug();
        if (snap_res.is_error()) return false;
        auto snapshot = snap_res.value();
        auto actor_res = runtime_.findActor(actor_key);
        if (actor_res.is_error()) return false;
        auto actor = *actor_res.value();

        std::vector<pathfinder::world_runtime::WorldEntityInstance> candidates;
        for (const auto& [entity_id, entity] : snapshot.entities) {
            if (entity.entity_key != input.object_key) continue;
            if (entity.location_kind != pathfinder::world_runtime::WorldEntityLocationKind::OnMap || !entity.coord) continue;
            if (!hasState(entity.state_keys, input.state)) continue;
            candidates.push_back(entity);
        }
        std::sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) {
            return manhattan(actor.coord, *a.coord) < manhattan(actor.coord, *b.coord);
        });
        for (const auto& entity : candidates) {
            if (!moveActorNear(actor_key, *entity.coord, changed_cells, changed_entities)) continue;
            pathfinder::world_inventory::InventoryTransferRequest req;
            req.transfer_id = actor_key + "_pickup_" + entity.entity_id;
            req.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::PickupFromMap;
            req.actor_key = actor_key;
            req.entity_id = entity.entity_id;
            req.quantity = std::max(1, std::min(missing, entity.quantity));
            auto transfer = inventory_runtime_.transfer(req);
            if (transfer.is_error() || !transfer.value().ok) continue;
            for (const auto& id : transfer.value().changed_cell_ids) changed_cells.insert(id);
            for (const auto& id : transfer.value().changed_entity_ids) changed_entities.insert(id);
            for (const auto& id : transfer.value().changed_inventory_ids) changed_inventories.insert(id);
            return true;
        }

        for (const auto& [node_id, node] : snapshot.resource_nodes) {
            if (node.remaining_charges <= 0 || node.node_state_str == "Depleted") continue;
            if (std::find(node.output_entity_keys.begin(), node.output_entity_keys.end(), input.object_key) == node.output_entity_keys.end()) continue;
            if (!moveActorNear(actor_key, node.coord, changed_cells, changed_entities)) continue;
            pathfinder::world_harvest::ResourceHarvestRequest req;
            req.harvest_id = actor_key + "_harvest_" + node.node_id;
            req.actor_key = actor_key;
            req.node_id = node.node_id;
            if (node.required_action_key == "gather") req.harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Gather;
            else if (node.required_action_key == "chop") req.harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Chop;
            else if (node.required_action_key == "mine") req.harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Mine;
            else if (node.required_action_key == "dig") req.harvest_kind = pathfinder::world_harvest::ResourceHarvestKind::Dig;
            else continue;
            auto harvest = harvest_service_.execute(req);
            if (!harvest.ok) continue;
            for (const auto& id : harvest.changed_cell_ids) changed_cells.insert(id);
            for (const auto& id : harvest.changed_entity_ids) changed_entities.insert(id);
            if (harvest.has_node_after) changed_entities.insert(harvest.node_after.node_id);
            result.events.insert(result.events.end(), harvest.events.begin(), harvest.events.end());
            return true;
        }

        return false;
    }

    void deliverOutputsToPlayer(const pathfinder::world_command::WorldCommandDto& command,
                                const pathfinder::content::ReactionDefinitionContent& reaction,
                                std::set<std::string>& changed_entities,
                                std::set<std::string>& changed_inventories,
                                pathfinder::world_command::WorldCommandExecutionResult& result) const {
        const std::string worker_actor_key = command.target.target_actor_key;
        for (const auto& output : reaction.outputs) {
            const int quantity = std::max(1, output.quantity_delta);
            pathfinder::world_inventory::InventoryOwnerRef worker_owner{pathfinder::world_inventory::InventoryOwnerKind::Actor, worker_actor_key};
            auto items_res = inventory_runtime_.queryItems(worker_owner, output.object_key);
            if (items_res.is_error() || items_res.value().empty()) continue;
            auto item = items_res.value().front();
            pathfinder::world_inventory::InventoryTransferRequest consume;
            consume.transfer_id = command.command_id + "_deliver_consume_" + output.object_key;
            consume.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::ConsumeToNowhere;
            consume.actor_key = worker_actor_key;
            consume.entity_id = item.entity_id;
            consume.quantity = std::min(quantity, item.quantity);
            auto consume_res = inventory_runtime_.transfer(consume);
            if (consume_res.is_error() || !consume_res.value().ok) continue;
            for (const auto& id : consume_res.value().changed_entity_ids) changed_entities.insert(id);
            for (const auto& id : consume_res.value().changed_inventory_ids) changed_inventories.insert(id);

            pathfinder::world_inventory::InventoryTransferRequest spawn;
            spawn.transfer_id = command.command_id + "_deliver_spawn_" + output.object_key;
            spawn.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::SpawnToInventory;
            spawn.actor_key = command.actor_key;
            spawn.entity_key = output.object_key;
            spawn.quantity = consume.quantity;
            spawn.parameters["stack_key"] = output.object_key + ":delivered";
            spawn.parameters["stackable"] = "true";
            if (const auto* object = content_registry_.findObject(output.object_key)) {
                spawn.parameters["display_name_key"] = object->display_key;
                spawn.parameters["tag_keys"] = joinStrings(object->safe_tags);
                spawn.parameters["numeric_states"] = numericStatesParam(object->default_numeric);
            }
            auto spawn_res = inventory_runtime_.transfer(spawn);
            if (spawn_res.is_error() || !spawn_res.value().ok) continue;
            for (const auto& id : spawn_res.value().changed_entity_ids) changed_entities.insert(id);
            for (const auto& id : spawn_res.value().changed_inventory_ids) changed_inventories.insert(id);
        }
    }

    void maybeCounterVisibleThreat(const std::string& worker_actor_key,
                                   const pathfinder::content::ReactionDefinitionContent& reaction,
                                   std::set<std::string>& changed_entities,
                                   pathfinder::world_command::WorldCommandExecutionResult& result,
                                   uint64_t tick) const {
        bool output_has_fire = false;
        for (const auto& output : reaction.outputs) {
            const auto* object = content_registry_.findObject(output.object_key);
            if (object && hasTag(object->safe_tags, "fire")) output_has_fire = true;
        }
        if (!output_has_fire) return;
        auto actor_res = runtime_.findActor(worker_actor_key);
        if (actor_res.is_error()) return;
        auto snap_res = runtime_.snapshotForDebug();
        if (snap_res.is_error()) return;
        for (const auto& [entity_id, entity] : snap_res.value().entities) {
            if (!entity.coord || manhattan(actor_res.value()->coord, *entity.coord) > 5) continue;
            if (!hasTag(entity.tag_keys, "danger") && !hasTag(entity.tag_keys, "predator")) continue;
            auto destroy = runtime_.destroyEntity(entity_id);
            if (destroy.is_error()) continue;
            changed_entities.insert(entity_id);
            pathfinder::world_command::WorldEventDto event;
            event.event_id = worker_actor_key + "_counter_threat_" + entity_id;
            event.event_kind = "NpcCounteredThreat";
            event.tick = tick;
            event.title_text = "NPC反击威胁";
            event.body_text = "NPC用刚掌握的火焰工具逼退了附近的威胁。";
            event.actor_key = worker_actor_key;
            event.target_entity_id = entity_id;
            result.events.push_back(std::move(event));
            break;
        }
    }

    void buildPatch(pathfinder::world_command::WorldCommandExecutionResult& result,
                    const std::string& viewer_actor_key,
                    const std::set<std::string>& changed_cells,
                    const std::set<std::string>& changed_entities,
                    const std::set<std::string>& changed_inventories,
                    uint64_t base_version) const {
        pathfinder::world_command::WorldProjectionPatchDto patch;
        patch.base_projection_version = base_version;
        patch.new_projection_version = base_version + 1;
        pathfinder::world_runtime::WorldProjectionAdapter world_adapter;
        pathfinder::world_inventory::InventoryProjectionAdapter inventory_adapter;
        std::vector<std::string> cell_ids(changed_cells.begin(), changed_cells.end());
        std::vector<std::string> entity_ids(changed_entities.begin(), changed_entities.end());
        std::vector<std::string> inventory_ids(changed_inventories.begin(), changed_inventories.end());
        auto cell_patches = world_adapter.buildCellPatches(cell_ids, viewer_actor_key, runtime_);
        if (cell_patches.is_ok()) patch.changed_cells = std::move(cell_patches.value());
        auto entity_patches = world_adapter.buildEntityPatches(entity_ids, viewer_actor_key, runtime_);
        if (entity_patches.is_ok()) patch.changed_entities = std::move(entity_patches.value());
        auto inv_patches = inventory_adapter.buildInventoryPatches(inventory_ids, viewer_actor_key, inventory_runtime_);
        if (inv_patches.is_ok()) patch.changed_inventories = std::move(inv_patches.value());
        result.projection_patch_override = std::move(patch);
    }
};

pathfinder::foundation::Result<void> initializePlayableWorld(
    WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_generation::WorldGenerationService& worldgen_service,
    pathfinder::world_region_state::InMemoryWorldRegionStateStore* region_store,
    pathfinder::world_map_interaction::RegionActivityWindowService* activity_window_service,
    const pathfinder::content::ContentRegistry* content_registry) {

    WorldRuntimeConfig config;
    config.world_id = "world_default";
    config.seed = 1;
    config.region_size = 16;
    config.default_vision_radius = 10;

    auto init_res = runtime.initialize(config);
    if (init_res.is_error()) {
        return init_res;
    }

    world_generation::WorldGenerationRequest wg_request;
    wg_request.world_id = config.world_id;
    wg_request.world_seed = config.seed;
    wg_request.generator_version = "1.0.0";
    wg_request.content_version = "1.0.0";
    wg_request.worldgen_profile_key = "first_world";
    wg_request.region_coord = world_generation::WorldRegionCoord{0, 0};
    wg_request.region_size = config.region_size;
    wg_request.enabled_layer_keys = std::vector<std::string>{"surface"};

    auto wg_result = worldgen_service.generate(wg_request);
    if (!wg_result.ok || wg_result.cell_drafts.empty()) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::state_change_invalid,
                "origin_region_generation_failed", "Failed to generate origin region"));
    }

    world_generation::WorldGenerationApplier applier(runtime, runtime);
    auto apply_result = applier.apply(wg_result);
    if (!apply_result.ok) {
        return pathfinder::foundation::Result<void>::fail(
            pathfinder::foundation::makeError(pathfinder::foundation::ErrorCode::state_change_invalid,
                "origin_region_apply_failed", "Failed to apply origin region"));
    }

    auto player_res = runtime.setupPlayerActor(config);
    if (player_res.is_error()) {
        return player_res;
    }

    auto inv_init = inventory_runtime.initialize();
    if (inv_init.is_error()) {
        return inv_init;
    }

    if (content_registry) {
        auto spawn_object = [&](const std::string& entity_id, const std::string& object_key, const WorldCellCoord& preferred) {
            auto coord = nearestOpenCoord(runtime, preferred, 8);
            if (!coord) return;
            const auto* object = content_registry->findObject(object_key);
            const std::string display = object ? object->display_key : "object." + object_key + ".name";
            const auto tags = object ? object->safe_tags : std::vector<std::string>{};
            const auto numeric = object ? object->default_numeric : std::map<std::string, double>{};
            (void)runtime.spawnEntityOnMap(
                entity_id, object_key, display, *coord, 1, object_key + ":scenario", true, {}, numeric, tags);
        };

        if (auto coord = nearestOpenCoord(runtime, WorldCellCoord{1, 0, "surface"}, 8)) {
            (void)runtime.spawnActor("companion", "companion", "agent.companion.name", *coord, config.default_vision_radius, false);
        }
        spawn_object("scenario_camp_fire", "camp_fire", WorldCellCoord{2, 0, "surface"});
        spawn_object("scenario_beast_shadow", "beast_shadow", WorldCellCoord{4, 1, "surface"});
    }

    if (region_store) region_store->clear();
    if (activity_window_service) activity_window_service->clearTracking();

    return pathfinder::foundation::Result<void>::ok();
}

std::mutex& resetMutex() {
    static std::mutex mutex;
    return mutex;
}

std::filesystem::path findContentRoot() {
    auto find_from = [](std::filesystem::path current) -> std::filesystem::path {
        for (int i = 0; i < 10; ++i) {
            auto content_root = current / "content";
            if (std::filesystem::exists(content_root / "core" / "manifest.json")) {
                return content_root;
            }
            if (!current.has_parent_path() || current == current.parent_path()) break;
            current = current.parent_path();
        }
        return {};
    };

    if (auto from_cwd = find_from(std::filesystem::current_path()); !from_cwd.empty()) {
        return from_cwd;
    }

#ifdef PATHFINDER_REPO_ROOT
    {
        auto content_root = std::filesystem::path(PATHFINDER_REPO_ROOT) / "content";
        if (std::filesystem::exists(content_root / "core" / "manifest.json")) {
            return content_root;
        }
    }
#endif

    if (auto from_source = find_from(std::filesystem::path(__FILE__).parent_path()); !from_source.empty()) {
        return from_source;
    }

    return std::filesystem::path("content");
}


} // namespace

ClientServerRuntimeFactory::ClientServerRuntimeFactory()
    : world_runtime(std::make_shared<WorldGridRuntime>())
    , inventory_runtime(std::make_shared<pathfinder::world_inventory::WorldInventoryRuntime>(*world_runtime))
    , harvest_service(std::make_shared<pathfinder::world_harvest::ResourceHarvestService>(*world_runtime, *world_runtime, *inventory_runtime))
    , projection_bridge(std::make_shared<ClientRuntimeProjectionBridge>(*world_runtime, inventory_runtime.get(), ClientRuntimeBridgeMode::RuntimeBacked))
    , option_bridge(std::make_shared<ClientRuntimeCommandOptionBridge>(*world_runtime, registry, harvest_service.get(), inventory_runtime.get(), ClientRuntimeBridgeMode::RuntimeBacked))
    , projection_adapter(projection_bridge)
    , available_command_adapter(option_bridge)
    , session_gateway(projection_adapter, available_command_adapter)
    , dispatcher(registry)
    , pipeline(dispatcher)
    , command_gateway(session_gateway, pipeline, patch_contract, available_command_adapter) {

    // P58/P62: Create world generation service and load content-backed profiles.
    worldgen_service = std::make_shared<world_generation::WorldGenerationService>();

    {
        pathfinder::content::JsonContentLoader loader;
        pathfinder::content::ContentLoadOptions options;
        options.root_path = findContentRoot().string();
        options.enabled_package_keys = {"core"};
        options.load_mode = pathfinder::content::ContentLoadMode::StrictContentRequired;
        auto content_load = loader.load(options);
        if (content_load.is_ok() && content_load.value().registry) {
            content_registry = content_load.value().registry;
            harvest_service->setContentRegistry(content_registry);
            auto register_res = worldgen_service->registerContentProfiles(*content_registry);
            if (register_res.is_error()) {
                throw std::runtime_error("[P62] Failed to register content worldgen profiles");
            }
        } else {
            throw std::runtime_error("[P62] Failed to load required core content registry");
        }
    }

    reaction_service = std::make_shared<pathfinder::world_reaction::WorldReactionService>(
        *content_registry, *world_runtime, *inventory_runtime, *world_runtime);
    option_bridge->setCraftServices(reaction_service.get(), content_registry);

    learning_service = std::make_shared<pathfinder::learning::LearningLoopService>();
    knowledge_repository = std::make_shared<pathfinder::knowledge::KnowledgeRepository>();
    knowledge_learning_service = std::make_shared<pathfinder::world_learning::WorldKnowledgeLearningService>(
        *content_registry, *learning_service, *knowledge_repository);
    option_bridge->setKnowledgeServices(knowledge_repository.get(), content_registry);

    command_gateway.setPostCommandHook([this](const WorldCommandDto& command, ClientCommandResponse& response) {
        if (!knowledge_learning_service || !knowledge_repository || response.experiences.empty()) {
            return;
        }

        pathfinder::world_learning::WorldKnowledgeLearningRequest request;
        request.request_id = response.result.command_id + "_learning";
        request.source_command = command;
        request.source_kind = pathfinder::world_learning::WorldLearningSourceKind::DirectExperience;
        request.actor_key = response.result.actor_key.empty() ? command.actor_key : response.result.actor_key;
        request.tick = response.new_projection_version;
        request.command_result.result_kind = response.result.result_kind;
        request.command_result.failure_reason_keys = response.result.failure_reason_keys;
        request.command_result.warning_keys = response.result.warning_keys;
        request.command_result.state_deltas = response.result.state_deltas;
        request.command_result.spawned_commands = response.result.spawned_commands;
        request.command_result.events = response.event_feed;
        request.command_result.experiences = response.experiences;

        pathfinder::knowledge::KnowledgeOwner owner;
        owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
        owner.entity_id = pathfinder::foundation::EntityId(request.actor_key);
        auto claims_res = knowledge_repository->listByOwner(owner);
        if (claims_res.is_ok()) {
            request.existing_actor_claims = claims_res.value();
        }

        auto learning_res = knowledge_learning_service->learnFromCommandResult(request);
        if (learning_res.is_error()) {
            response.warning_keys.push_back("knowledge_learning_failed");
            return;
        }
        auto learning = learning_res.value();
        response.warning_keys.insert(response.warning_keys.end(), learning.warning_keys.begin(), learning.warning_keys.end());
        response.event_feed.insert(response.event_feed.end(), learning.events.begin(), learning.events.end());
        response.result.state_deltas.insert(response.result.state_deltas.end(), learning.state_deltas.begin(), learning.state_deltas.end());
        if (learning.projection_patch.has_value()) {
            auto& patch = learning.projection_patch.value();
            response.projection_patch.changed_knowledge.insert(
                response.projection_patch.changed_knowledge.end(),
                patch.changed_knowledge.begin(),
                patch.changed_knowledge.end());
        }
    });

    ensure_service = std::make_shared<world_generation::WorldRegionEnsureService>(
        *worldgen_service, *world_runtime, *world_runtime);
    ensure_adapter = std::make_shared<client_runtime_bridge::ClientWorldRegionEnsureAdapter>(
        *ensure_service, *world_runtime);
    move_guard = std::make_shared<world_generation::MoveTargetRegionGuard>(*ensure_service);

    // P59/P60: Create region lifecycle infrastructure
    region_store = std::make_shared<pathfinder::world_region_state::InMemoryWorldRegionStateStore>();
    apply_port = std::make_shared<pathfinder::world_region_state::WorldGridRuntimeApplyPort>(*world_runtime, *world_runtime);
    ensure_service_adapter = std::make_shared<pathfinder::world_region_state::WorldRegionEnsureServiceAdapter>(*ensure_service);
    lifecycle_service = std::make_shared<pathfinder::world_region_state::WorldRegionLifecycleService>(
        *region_store, *world_runtime, *apply_port, *world_runtime, *world_runtime, ensure_service_adapter.get());

    // P60: Wire lifecycle service into ensure adapter so restores are reported correctly.
    ensure_adapter->setLifecycleService(lifecycle_service.get());

    // P58: Inject ensure adapter into session gateway and command gateway.
    session_gateway.setRegionEnsureAdapter(ensure_adapter.get());
    command_gateway.setRegionEnsureAdapter(ensure_adapter.get());

    // Register runtime-aware handlers (P56)
    registry.registerHandler(createGenerateWorldCommandHandler(worldgen_service, *world_runtime, *world_runtime));
    registry.registerHandler(createMoveCommandHandler(*world_runtime, move_guard.get()));
    registry.registerHandler(std::make_shared<NpcKnowledgeInspectCommandHandler>(*world_runtime, *knowledge_repository, *content_registry));
    registry.registerHandler(createWaitCommandHandler(*world_runtime));

    // P60: Register harvest handlers
    registry.registerHandler(pathfinder::world_command::createGatherCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createChopCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createMineCommandHandler(*harvest_service));
    registry.registerHandler(pathfinder::world_command::createDigCommandHandler(*harvest_service));

    // P60: Register inventory handlers
    registry.registerHandler(pathfinder::world_inventory::createPickupCommandHandler(*inventory_runtime, *world_runtime));
    registry.registerHandler(pathfinder::world_inventory::createDropCommandHandler(*inventory_runtime, *world_runtime));

    // P48/P63: Register content-backed craft handler.
    registry.registerHandler(pathfinder::world_reaction::createCraftCommandHandler(*reaction_service, *world_runtime, *inventory_runtime));

    registry.registerHandler(pathfinder::world_teaching::createTeachCommandHandler(
        *knowledge_repository, std::make_unique<RuntimeActorQueryPort>(*world_runtime), content_registry.get()));
    registry.registerHandler(std::make_shared<NpcWorkUseCommandHandler>(
        *world_runtime, *inventory_runtime, *harvest_service, *reaction_service, *content_registry, *knowledge_repository));

    // Register base system handlers. These are not gameplay fallbacks and should
    // not be used to fake missing content actions; real playable behavior must have
    // a runtime-aware handler/provider wired through the Command pipeline.
    registry.registerHandler(createNoopCommandHandler());
    registry.registerHandler(createSystemTickCommandHandler());

    // P60: Create region activity window service
    {
        world_map_interaction::RegionActivityWindowService::Config aw_config;
        aw_config.vision_radius_cells = 10;
        aw_config.preload_margin_regions = 1;
        aw_config.seal_delay_ticks = 10;
        aw_config.detach_delay_ticks = 3;
        aw_config.recently_touched_window_ticks = 10;
        activity_window_service = std::make_shared<world_map_interaction::RegionActivityWindowService>(
            aw_config, *world_runtime);
    }

    // P60: Create region lifecycle trigger service
    lifecycle_trigger_service = std::make_shared<world_map_interaction::RegionLifecycleTriggerService>(
        *lifecycle_service);
    lifecycle_trigger_service->setActivityWindowService(activity_window_service.get());

    auto world_reset_res = resetWorld();
    if (world_reset_res.is_error()) {
        std::cerr << "[P60] Initial playable world reset failed\n";
    }

    // P60: Create map selection service
    map_selection_service = std::make_shared<world_map_interaction::ClientMapSelectionService>(
        *world_runtime, *world_runtime, available_command_adapter);

    // P60: Create map projection adapter
    map_projection_adapter = std::make_shared<world_map_interaction::ClientMapProjectionAdapter>(
        projection_adapter, *world_runtime);

    // P60: Inject activity window and lifecycle trigger services into gateways.
    session_gateway.setActivityWindowService(activity_window_service.get());
    session_gateway.setLifecycleTriggerService(lifecycle_trigger_service.get());
    session_gateway.setWorldContext("world_default", 1);
    command_gateway.setActivityWindowService(activity_window_service.get());
    command_gateway.setLifecycleTriggerService(lifecycle_trigger_service.get());
    command_gateway.setWorldContext("world_default", 1);

    // P60: Create HTTP gateway with selection service
    http_gateway = std::make_unique<ClientHttpGateway>(
        codec,
        session_gateway,
        command_gateway,
        map_selection_service.get(),
        [this]() { return this->resetWorld(); });

}

pathfinder::foundation::Result<void> ClientServerRuntimeFactory::resetWorld() {
    std::lock_guard<std::mutex> lock(resetMutex());
    if (knowledge_repository) {
        knowledge_repository->clear();
    }
    return initializePlayableWorld(
        *world_runtime,
        *inventory_runtime,
        *worldgen_service,
        region_store.get(),
        activity_window_service.get(),
        content_registry.get());
}

} // namespace pathfinder::client_http
