#include "pathfinder/world_modules/npc_work/world_npc_work_module.h"
#include "pathfinder/world_modules/core/world_module_utils.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace pathfinder::world_npc_work {
namespace {

using pathfinder::world_modules::actorKnowledgeOwner;
using pathfinder::world_modules::hasState;
using pathfinder::world_modules::hasTag;
using pathfinder::world_modules::joinStrings;
using pathfinder::world_modules::manhattan;
using pathfinder::world_modules::numericStatesParam;
using pathfinder::world_modules::recipeInputQuantity;
using pathfinder::world_modules::sameCoord;
using pathfinder::world_modules::sameOrAdjacent8;

struct NpcWorkTask {
    std::string task_id;
    std::string requester_actor_key;
    std::string worker_actor_key;
    std::string reaction_key;
    size_t next_input_index = 0;
    std::string phase = "acquire_inputs";
};

std::map<std::string, NpcWorkTask>& npcWorkTasks() {
    static std::map<std::string, NpcWorkTask> tasks;
    return tasks;
}

std::map<std::string, std::string>& activeNpcWorkTaskByWorker() {
    static std::map<std::string, std::string> active;
    return active;
}

bool workerKnowsRecipe(pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
                       const std::string& actor_key,
                       const pathfinder::content::ReactionDefinitionContent& reaction) {
    auto claims_res = knowledge_repository.listByOwner(actorKnowledgeOwner(actor_key));
    if (claims_res.is_error()) return false;
    for (const auto& claim : claims_res.value()) {
        if (!claim.projection_flags.usable_by_ai) continue;
        if ((claim.predicate.action_key == reaction.action_key || claim.predicate.action_key == "craft") &&
            claim.predicate.effect_key == reaction.effect_key) return true;
    }
    return false;
}

class NpcWorkUseCommandHandler final : public pathfinder::world_modules::IWorldUseCommandHandler {
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

    bool supports(const pathfinder::world_command::WorldCommandDto& command) const override {
        return command.command_key == "order_actor_craft";
    }

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override {
        using namespace pathfinder::world_command;
        (void)harvest_service_;
        (void)reaction_service_;
        WorldCommandExecutionResult result;
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

        if (!workerKnowsRecipe(knowledge_repository_, worker_actor_key, *reaction)) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_knowledge_missing");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        auto ensure_inv = inventory_runtime_.ensureActorInventory(worker_actor_key);
        if (ensure_inv.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("npc_inventory_missing");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        if (activeNpcWorkTaskByWorker().find(worker_actor_key) != activeNpcWorkTaskByWorker().end()) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_work_already_active");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        NpcWorkTask task;
        task.task_id = command.command_id + "_npc_work_task";
        task.requester_actor_key = command.actor_key;
        task.worker_actor_key = worker_actor_key;
        task.reaction_key = reaction_key;
        npcWorkTasks()[task.task_id] = task;
        activeNpcWorkTaskByWorker()[worker_actor_key] = task.task_id;

        WorldEventDto event;
        event.event_id = command.command_id + "_npc_work_accepted";
        event.event_kind = "NpcWorkOrderAccepted";
        event.tick = context.currentTick();
        event.title_text = "NPC接受代工";
        event.body_text = "NPC接受了任务，会在后续时间里逐步采集、制作并交付。";
        event.actor_key = worker_actor_key;
        result.events.push_back(std::move(event));

        result.result_kind = WorldCommandResultKind::Succeeded;
        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    pathfinder::world_runtime::WorldGridRuntime& runtime_;
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime_;
    pathfinder::world_harvest::ResourceHarvestService& harvest_service_;
    pathfinder::world_reaction::WorldReactionService& reaction_service_;
    const pathfinder::content::ContentRegistry& content_registry_;
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository_;
};

enum class NpcWorkStepOutcome {
    None,
    Progressed,
    Completed,
    Blocked
};

bool npcReactionConsumesInput(const pathfinder::content::ReactionDefinitionContent& reaction,
                              const std::string& object_key,
                              const std::string& state) {
    for (const auto& consume : reaction.consume) {
        if (consume.object_key != object_key) continue;
        if (!state.empty() && !consume.state.empty() && consume.state != state) continue;
        return true;
    }
    return false;
}

int npcInventoryQuantity(pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                         const std::string& actor_key,
                         const std::string& object_key,
                         const std::string& state) {
    pathfinder::world_inventory::InventoryOwnerRef owner{pathfinder::world_inventory::InventoryOwnerKind::Actor, actor_key};
    auto items = inventory_runtime.queryItems(owner, object_key);
    if (items.is_error()) return 0;
    int total = 0;
    for (const auto& item : items.value()) {
        if (hasState(item.state_keys, state)) total += item.quantity;
    }
    return total;
}

bool npcWorkerKnowsRecipe(pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
                          const std::string& actor_key,
                          const pathfinder::content::ReactionDefinitionContent& reaction) {
    auto claims_res = knowledge_repository.listByOwner(actorKnowledgeOwner(actor_key));
    if (claims_res.is_error()) return false;
    for (const auto& claim : claims_res.value()) {
        if (!claim.projection_flags.usable_by_ai) continue;
        if ((claim.predicate.action_key == reaction.action_key || claim.predicate.action_key == "craft") &&
            claim.predicate.effect_key == reaction.effect_key) return true;
    }
    return false;
}

void appendNpcWorkEvent(pathfinder::client_protocol::ClientCommandResponse& response,
                        const NpcWorkTask& task,
                        const std::string& kind,
                        const std::string& title,
                        const std::string& body) {
    pathfinder::world_command::WorldEventDto event;
    event.event_id = response.result.command_id + "_" + task.task_id + "_" + kind + "_" + std::to_string(response.event_feed.size());
    event.event_kind = kind;
    event.tick = response.new_projection_version;
    event.title_text = title;
    event.body_text = body;
    event.actor_key = task.worker_actor_key;
    response.event_feed.push_back(std::move(event));
}

void appendNpcWorkProjectionChanges(pathfinder::client_protocol::ClientCommandResponse& response,
                                    const std::string& viewer_actor_key,
                                    pathfinder::world_runtime::WorldGridRuntime& runtime,
                                    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                                    const std::set<std::string>& changed_cells,
                                    const std::set<std::string>& changed_entities,
                                    const std::set<std::string>& changed_inventories) {
    pathfinder::world_runtime::WorldProjectionAdapter world_adapter;
    pathfinder::world_inventory::InventoryProjectionAdapter inventory_adapter;
    std::vector<std::string> cell_ids(changed_cells.begin(), changed_cells.end());
    std::vector<std::string> entity_ids(changed_entities.begin(), changed_entities.end());
    std::vector<std::string> inventory_ids(changed_inventories.begin(), changed_inventories.end());

    auto cell_patches = world_adapter.buildCellPatches(cell_ids, viewer_actor_key, runtime);
    if (cell_patches.is_ok()) {
        response.projection_patch.changed_cells.insert(
            response.projection_patch.changed_cells.end(), cell_patches.value().begin(), cell_patches.value().end());
    }
    auto entity_patches = world_adapter.buildEntityPatches(entity_ids, viewer_actor_key, runtime);
    if (entity_patches.is_ok()) {
        response.projection_patch.changed_entities.insert(
            response.projection_patch.changed_entities.end(), entity_patches.value().begin(), entity_patches.value().end());
    }
    auto inv_patches = inventory_adapter.buildInventoryPatches(inventory_ids, viewer_actor_key, inventory_runtime);
    if (inv_patches.is_ok()) {
        response.projection_patch.changed_inventories.insert(
            response.projection_patch.changed_inventories.end(), inv_patches.value().begin(), inv_patches.value().end());
    }
}

enum class NpcMoveOneStepResult { AlreadyNear, Moved, Blocked };

NpcMoveOneStepResult moveActorOneStepNear(pathfinder::world_runtime::WorldGridRuntime& runtime,
                                          const std::string& actor_key,
                                          const pathfinder::world_runtime::WorldCellCoord& target,
                                          std::set<std::string>& changed_cells,
                                          std::set<std::string>& changed_entities) {
    auto actor_res = runtime.findActor(actor_key);
    if (actor_res.is_error()) return NpcMoveOneStepResult::Blocked;
    auto actor = *actor_res.value();
    if (sameOrAdjacent8(actor.coord, target)) return NpcMoveOneStepResult::AlreadyNear;

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
    candidates.erase(std::unique(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
        return sameCoord(left, right);
    }), candidates.end());

    for (const auto& candidate : candidates) {
        auto move = runtime.moveActor(actor_key, candidate);
        if (move.is_error() || !move.value().moved) continue;
        for (const auto& id : move.value().changed_cell_ids) changed_cells.insert(id);
        for (const auto& id : move.value().changed_entity_ids) changed_entities.insert(id);
        return NpcMoveOneStepResult::Moved;
    }
    return NpcMoveOneStepResult::Blocked;
}

std::optional<pathfinder::world_harvest::ResourceHarvestKind> harvestKindFromActionKey(const std::string& action_key) {
    if (action_key == "gather") return pathfinder::world_harvest::ResourceHarvestKind::Gather;
    if (action_key == "chop") return pathfinder::world_harvest::ResourceHarvestKind::Chop;
    if (action_key == "mine") return pathfinder::world_harvest::ResourceHarvestKind::Mine;
    if (action_key == "dig") return pathfinder::world_harvest::ResourceHarvestKind::Dig;
    return std::nullopt;
}

NpcWorkStepOutcome advanceNpcWorkAcquireInput(NpcWorkTask& task,
                                              const pathfinder::content::ReactionInputDto& input,
                                              int missing,
                                              bool consumes_input,
                                              pathfinder::world_runtime::WorldGridRuntime& runtime,
                                              pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                                              pathfinder::world_harvest::ResourceHarvestService& harvest_service,
                                              pathfinder::client_protocol::ClientCommandResponse& response,
                                              std::set<std::string>& changed_cells,
                                              std::set<std::string>& changed_entities,
                                              std::set<std::string>& changed_inventories) {
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return NpcWorkStepOutcome::Blocked;
    auto snapshot = snap_res.value();
    auto actor_res = runtime.findActor(task.worker_actor_key);
    if (actor_res.is_error()) return NpcWorkStepOutcome::Blocked;
    const auto actor = *actor_res.value();

    std::vector<pathfinder::world_runtime::WorldEntityInstance> item_candidates;
    for (const auto& [entity_id, entity] : snapshot.entities) {
        if (entity.entity_key != input.object_key) continue;
        if (entity.location_kind != pathfinder::world_runtime::WorldEntityLocationKind::OnMap || !entity.coord) continue;
        if (!hasState(entity.state_keys, input.state)) continue;
        item_candidates.push_back(entity);
    }
    std::sort(item_candidates.begin(), item_candidates.end(), [&](const auto& a, const auto& b) {
        return manhattan(actor.coord, *a.coord) < manhattan(actor.coord, *b.coord);
    });
    if (!item_candidates.empty()) {
        const auto& entity = item_candidates.front();
        const auto move = moveActorOneStepNear(runtime, task.worker_actor_key, *entity.coord, changed_cells, changed_entities);
        if (move == NpcMoveOneStepResult::Moved) {
            appendNpcWorkEvent(response, task, "NpcWorkMoving", "NPC前往材料", "NPC正在前往需要的材料。 ");
            return NpcWorkStepOutcome::Progressed;
        }
        if (move == NpcMoveOneStepResult::Blocked) return NpcWorkStepOutcome::Blocked;
        if (!consumes_input) {
            task.next_input_index += 1;
            appendNpcWorkEvent(response, task, "NpcWorkMoving", "NPC确认条件", "NPC已经靠近需要的条件对象。只确认条件，不拾取。 ");
            return NpcWorkStepOutcome::Progressed;
        }

        pathfinder::world_inventory::InventoryTransferRequest req;
        req.transfer_id = task.task_id + "_pickup_" + entity.entity_id + "_" + std::to_string(response.new_projection_version);
        req.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::PickupFromMap;
        req.actor_key = task.worker_actor_key;
        req.entity_id = entity.entity_id;
        req.quantity = std::max(1, std::min(missing, entity.quantity));
        auto transfer = inventory_runtime.transfer(req);
        if (transfer.is_error() || !transfer.value().ok) return NpcWorkStepOutcome::Blocked;
        for (const auto& id : transfer.value().changed_cell_ids) changed_cells.insert(id);
        for (const auto& id : transfer.value().changed_entity_ids) changed_entities.insert(id);
        for (const auto& id : transfer.value().changed_inventory_ids) changed_inventories.insert(id);
        appendNpcWorkEvent(response, task, "NpcWorkPicking", "NPC拾取材料", "NPC拾取了一份材料。 ");
        return NpcWorkStepOutcome::Progressed;
    }

    if (!consumes_input) return NpcWorkStepOutcome::Blocked;

    std::vector<pathfinder::world_runtime::WorldResourceNodeRuntime> node_candidates;
    for (const auto& [node_id, node] : snapshot.resource_nodes) {
        if (node.remaining_charges <= 0 || node.node_state_str == "Depleted") continue;
        if (std::find(node.output_entity_keys.begin(), node.output_entity_keys.end(), input.object_key) == node.output_entity_keys.end()) continue;
        node_candidates.push_back(node);
    }
    std::sort(node_candidates.begin(), node_candidates.end(), [&](const auto& a, const auto& b) {
        return manhattan(actor.coord, a.coord) < manhattan(actor.coord, b.coord);
    });
    if (!node_candidates.empty()) {
        const auto& node = node_candidates.front();
        const auto move = moveActorOneStepNear(runtime, task.worker_actor_key, node.coord, changed_cells, changed_entities);
        if (move == NpcMoveOneStepResult::Moved) {
            appendNpcWorkEvent(response, task, "NpcWorkMoving", "NPC前往资源", "NPC正在前往可采集资源。 ");
            return NpcWorkStepOutcome::Progressed;
        }
        if (move == NpcMoveOneStepResult::Blocked) return NpcWorkStepOutcome::Blocked;

        auto harvest_kind = harvestKindFromActionKey(node.required_action_key);
        if (!harvest_kind) return NpcWorkStepOutcome::Blocked;
        pathfinder::world_harvest::ResourceHarvestRequest req;
        req.harvest_id = task.task_id + "_harvest_" + node.node_id + "_" + std::to_string(response.new_projection_version);
        req.actor_key = task.worker_actor_key;
        req.node_id = node.node_id;
        req.harvest_kind = *harvest_kind;
        auto harvest = harvest_service.execute(req);
        if (!harvest.ok) return NpcWorkStepOutcome::Blocked;
        for (const auto& id : harvest.changed_cell_ids) changed_cells.insert(id);
        for (const auto& id : harvest.changed_entity_ids) changed_entities.insert(id);
        if (harvest.has_node_after) changed_entities.insert(harvest.node_after.node_id);
        response.event_feed.insert(response.event_feed.end(), harvest.events.begin(), harvest.events.end());
        appendNpcWorkEvent(response, task, "NpcWorkGathering", "NPC采集材料", "NPC采集了一次资源，掉落物需要后续拾取。 ");
        return NpcWorkStepOutcome::Progressed;
    }

    return NpcWorkStepOutcome::Blocked;
}

void deliverNpcWorkOutputsToPlayer(const NpcWorkTask& task,
                                   const pathfinder::content::ReactionDefinitionContent& reaction,
                                   const pathfinder::content::ContentRegistry& content_registry,
                                   pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                                   std::set<std::string>& changed_entities,
                                   std::set<std::string>& changed_inventories) {
    for (const auto& output : reaction.outputs) {
        const int quantity = std::max(1, output.quantity_delta);
        pathfinder::world_inventory::InventoryOwnerRef worker_owner{pathfinder::world_inventory::InventoryOwnerKind::Actor, task.worker_actor_key};
        auto items_res = inventory_runtime.queryItems(worker_owner, output.object_key);
        if (items_res.is_error() || items_res.value().empty()) continue;
        auto item = items_res.value().front();

        pathfinder::world_inventory::InventoryTransferRequest consume;
        consume.transfer_id = task.task_id + "_deliver_consume_" + output.object_key;
        consume.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::ConsumeToNowhere;
        consume.actor_key = task.worker_actor_key;
        consume.entity_id = item.entity_id;
        consume.quantity = std::min(quantity, item.quantity);
        auto consume_res = inventory_runtime.transfer(consume);
        if (consume_res.is_error() || !consume_res.value().ok) continue;
        for (const auto& id : consume_res.value().changed_entity_ids) changed_entities.insert(id);
        for (const auto& id : consume_res.value().changed_inventory_ids) changed_inventories.insert(id);

        pathfinder::world_inventory::InventoryTransferRequest spawn;
        spawn.transfer_id = task.task_id + "_deliver_spawn_" + output.object_key;
        spawn.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::SpawnToInventory;
        spawn.actor_key = task.requester_actor_key;
        spawn.entity_key = output.object_key;
        spawn.quantity = consume.quantity;
        spawn.parameters["stack_key"] = output.object_key + ":delivered";
        spawn.parameters["stackable"] = "true";
        if (const auto* object = content_registry.findObject(output.object_key)) {
            spawn.parameters["display_name_key"] = object->display_key;
            spawn.parameters["tag_keys"] = joinStrings(object->safe_tags);
            spawn.parameters["numeric_states"] = numericStatesParam(object->default_numeric);
        }
        auto spawn_res = inventory_runtime.transfer(spawn);
        if (spawn_res.is_error() || !spawn_res.value().ok) continue;
        for (const auto& id : spawn_res.value().changed_entity_ids) changed_entities.insert(id);
        for (const auto& id : spawn_res.value().changed_inventory_ids) changed_inventories.insert(id);
    }
}

void maybeNpcWorkCounterVisibleThreat(const NpcWorkTask& task,
                                      const pathfinder::content::ReactionDefinitionContent& reaction,
                                      const pathfinder::content::ContentRegistry& content_registry,
                                      pathfinder::world_runtime::WorldGridRuntime& runtime,
                                      pathfinder::client_protocol::ClientCommandResponse& response,
                                      std::set<std::string>& changed_entities) {
    bool output_has_fire = false;
    for (const auto& output : reaction.outputs) {
        const auto* object = content_registry.findObject(output.object_key);
        if (object && hasTag(object->safe_tags, "fire")) output_has_fire = true;
    }
    if (!output_has_fire) return;
    auto actor_res = runtime.findActor(task.worker_actor_key);
    if (actor_res.is_error()) return;
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return;
    for (const auto& [entity_id, entity] : snap_res.value().entities) {
        if (!entity.coord || manhattan(actor_res.value()->coord, *entity.coord) > 5) continue;
        if (!hasTag(entity.tag_keys, "danger") && !hasTag(entity.tag_keys, "predator")) continue;
        auto destroy = runtime.destroyEntity(entity_id);
        if (destroy.is_error()) continue;
        changed_entities.insert(entity_id);
        appendNpcWorkEvent(response, task, "NpcCounteredThreat", "NPC反击威胁", "NPC用刚掌握的火焰工具逼退了附近的威胁。 ");
        break;
    }
}

NpcWorkStepOutcome advanceNpcWorkTask(NpcWorkTask& task,
                                      pathfinder::world_runtime::WorldGridRuntime& runtime,
                                      pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                                      pathfinder::world_harvest::ResourceHarvestService& harvest_service,
                                      pathfinder::world_reaction::WorldReactionService& reaction_service,
                                      const pathfinder::content::ContentRegistry& content_registry,
                                      pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
                                      pathfinder::client_protocol::ClientCommandResponse& response) {
    const auto* reaction = content_registry.findReaction(task.reaction_key);
    if (!reaction) return NpcWorkStepOutcome::Blocked;
    if (!npcWorkerKnowsRecipe(knowledge_repository, task.worker_actor_key, *reaction)) return NpcWorkStepOutcome::Blocked;

    std::set<std::string> changed_cells;
    std::set<std::string> changed_entities;
    std::set<std::string> changed_inventories;
    auto ensure_inv = inventory_runtime.ensureActorInventory(task.worker_actor_key);
    if (ensure_inv.is_error()) return NpcWorkStepOutcome::Blocked;
    changed_inventories.insert(ensure_inv.value());

    if (task.phase == "acquire_inputs") {
        while (task.next_input_index < reaction->inputs.size()) {
            const auto& input = reaction->inputs[task.next_input_index];
            const bool consumes_input = npcReactionConsumesInput(*reaction, input.object_key, input.state);
            const int required = recipeInputQuantity(input);
            const int available = npcInventoryQuantity(inventory_runtime, task.worker_actor_key, input.object_key, input.state);
            if ((consumes_input && available >= required) || (!consumes_input && available > 0)) {
                task.next_input_index += 1;
                continue;
            }
            const int missing = consumes_input ? std::max(1, required - available) : 1;
            auto outcome = advanceNpcWorkAcquireInput(
                task, input, missing, consumes_input, runtime, inventory_runtime, harvest_service,
                response, changed_cells, changed_entities, changed_inventories);
            appendNpcWorkProjectionChanges(response, task.requester_actor_key, runtime, inventory_runtime,
                                           changed_cells, changed_entities, changed_inventories);
            return outcome;
        }
        task.phase = "craft";
    }

    if (task.phase == "craft") {
        pathfinder::world_reaction::WorldReactionRequest craft_req;
        craft_req.reaction_command_id = task.task_id + "_npc_craft_" + std::to_string(response.new_projection_version);
        craft_req.actor_key = task.worker_actor_key;
        craft_req.reaction_key = task.reaction_key;
        craft_req.action_kind = pathfinder::world_reaction::WorldReactionActionKind::Craft;
        craft_req.output_location_policy = pathfinder::world_reaction::WorldReactionOutputLocationPolicy::ActorInventory;
        auto craft_res = reaction_service.execute(craft_req);
        if (craft_res.is_error() || !craft_res.value().ok) return NpcWorkStepOutcome::Blocked;
        auto craft = craft_res.value();
        for (const auto& id : craft.changed_cell_ids) changed_cells.insert(id);
        for (const auto& id : craft.changed_entity_ids) changed_entities.insert(id);
        for (const auto& id : craft.changed_inventory_ids) changed_inventories.insert(id);
        response.event_feed.insert(response.event_feed.end(), craft.events.begin(), craft.events.end());
        response.experiences.insert(response.experiences.end(), craft.experiences.begin(), craft.experiences.end());
        response.result.state_deltas.insert(response.result.state_deltas.end(), craft.state_deltas.begin(), craft.state_deltas.end());
        appendNpcWorkEvent(response, task, "NpcWorkCrafting", "NPC制作物品", "NPC按照已掌握知识完成了一次制作。 ");
        task.phase = "deliver";
        appendNpcWorkProjectionChanges(response, task.requester_actor_key, runtime, inventory_runtime,
                                       changed_cells, changed_entities, changed_inventories);
        return NpcWorkStepOutcome::Progressed;
    }

    if (task.phase == "deliver") {
        auto worker_res = runtime.findActor(task.worker_actor_key);
        auto requester_res = runtime.findActor(task.requester_actor_key);
        if (worker_res.is_error() || requester_res.is_error()) return NpcWorkStepOutcome::Blocked;
        if (!sameOrAdjacent8(worker_res.value()->coord, requester_res.value()->coord)) {
            auto move = moveActorOneStepNear(runtime, task.worker_actor_key, requester_res.value()->coord, changed_cells, changed_entities);
            if (move == NpcMoveOneStepResult::Moved) {
                appendNpcWorkEvent(response, task, "NpcWorkDelivering", "NPC交付途中", "NPC正在把产物送回给你。 ");
                appendNpcWorkProjectionChanges(response, task.requester_actor_key, runtime, inventory_runtime,
                                               changed_cells, changed_entities, changed_inventories);
                return NpcWorkStepOutcome::Progressed;
            }
            if (move == NpcMoveOneStepResult::Blocked) return NpcWorkStepOutcome::Blocked;
        }
        deliverNpcWorkOutputsToPlayer(task, *reaction, content_registry, inventory_runtime, changed_entities, changed_inventories);
        maybeNpcWorkCounterVisibleThreat(task, *reaction, content_registry, runtime, response, changed_entities);
        appendNpcWorkEvent(response, task, "NpcWorkOrderCompleted", "NPC完成代工", "NPC完成任务，并把产物交给了你。 ");
        appendNpcWorkProjectionChanges(response, task.requester_actor_key, runtime, inventory_runtime,
                                       changed_cells, changed_entities, changed_inventories);
        return NpcWorkStepOutcome::Completed;
    }

    return NpcWorkStepOutcome::Blocked;
}

void runNpcWorkTicksInternal(pathfinder::world_runtime::WorldGridRuntime& runtime,
                     pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
                     pathfinder::world_harvest::ResourceHarvestService& harvest_service,
                     pathfinder::world_reaction::WorldReactionService& reaction_service,
                     const pathfinder::content::ContentRegistry& content_registry,
                     pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
                     pathfinder::client_protocol::ClientCommandResponse& response) {
    if (npcWorkTasks().empty()) return;
    std::vector<std::string> task_ids;
    for (const auto& [task_id, task] : npcWorkTasks()) task_ids.push_back(task_id);

    for (const auto& task_id : task_ids) {
        auto it = npcWorkTasks().find(task_id);
        if (it == npcWorkTasks().end()) continue;
        auto outcome = advanceNpcWorkTask(
            it->second, runtime, inventory_runtime, harvest_service, reaction_service,
            content_registry, knowledge_repository, response);
        if (outcome == NpcWorkStepOutcome::Completed) {
            activeNpcWorkTaskByWorker().erase(it->second.worker_actor_key);
            npcWorkTasks().erase(it);
        } else if (outcome == NpcWorkStepOutcome::Blocked) {
            appendNpcWorkEvent(response, it->second, "NpcWorkBlocked", "NPC工作受阻", "NPC当前无法继续完成这个任务。 ");
            activeNpcWorkTaskByWorker().erase(it->second.worker_actor_key);
            npcWorkTasks().erase(it);
        }
    }
}



} // namespace

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createNpcWorkUseCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_harvest::ResourceHarvestService& harvest_service,
    pathfinder::world_reaction::WorldReactionService& reaction_service,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository) {
    return std::make_shared<NpcWorkUseCommandHandler>(
        runtime, inventory_runtime, harvest_service, reaction_service, content_registry, knowledge_repository);
}

void runNpcWorkTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    pathfinder::world_inventory::WorldInventoryRuntime& inventory_runtime,
    pathfinder::world_harvest::ResourceHarvestService& harvest_service,
    pathfinder::world_reaction::WorldReactionService& reaction_service,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::knowledge::KnowledgeRepository& knowledge_repository,
    pathfinder::client_protocol::ClientCommandResponse& response) {
    runNpcWorkTicksInternal(runtime, inventory_runtime, harvest_service, reaction_service, content_registry, knowledge_repository, response);
}

bool isNpcWorkActive(const std::string& actor_key) {
    return activeNpcWorkTaskByWorker().find(actor_key) != activeNpcWorkTaskByWorker().end();
}

void clearNpcWorkTasks() {
    npcWorkTasks().clear();
    activeNpcWorkTaskByWorker().clear();
}

} // namespace pathfinder::world_npc_work
