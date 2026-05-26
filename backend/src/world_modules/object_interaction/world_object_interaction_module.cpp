#include "pathfinder/world_modules/object_interaction/world_object_interaction_module.h"
#include "pathfinder/world_inventory/inventory_projection_adapter.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include "pathfinder/logging/logger.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace pathfinder::world_object_interaction {
namespace {

using pathfinder::content::ContentRegistry;
using pathfinder::content::FeedbackDefinitionContent;
using pathfinder::foundation::Result;
using pathfinder::world_command::IWorldCommandHandler;
using pathfinder::world_command::PatchOp;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldCommandContext;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandExecutionResult;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandResultKind;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::WorldEventDto;
using pathfinder::world_command::WorldExperienceDto;
using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::world_command::WorldStateDeltaDto;
using pathfinder::world_runtime::WorldCellCoord;
using pathfinder::world_runtime::WorldEntityInstance;
using pathfinder::world_runtime::WorldGridRuntime;
using pathfinder::world_runtime::WorldEntityLocationKind;

bool sameOrAdjacent(const WorldCellCoord& left, const WorldCellCoord& right) {
    if (left.layer_key != right.layer_key) return false;
    return std::abs(left.x - right.x) <= 1 && std::abs(left.y - right.y) <= 1;
}

std::string objectName(const ContentRegistry& registry, const std::string& object_key) {
    const auto key = "object." + object_key + ".name";
    auto translated = registry.translate("zh_cn", key);
    return translated == key ? object_key : translated;
}

void logObjectAction(const std::string& message) {
    pathfinder::logging::log(pathfinder::logging::tag::Command, "object_action " + message);
}

const FeedbackDefinitionContent* bestFeedback(
    const ContentRegistry& registry,
    const std::string& object_key,
    const std::string& action_key,
    const std::string& target_key) {
    auto matches = registry.feedbacksFor(object_key, action_key, target_key);
    return matches.empty() ? nullptr : matches.front();
}

WorldProjectionPatchDto buildObjectActionPatch(
    WorldGridRuntime& runtime,
    const pathfinder::world_inventory::IInventoryRuntime* inventory_runtime,
    const std::string& viewer_actor_key,
    const std::vector<std::string>& changed_cell_ids,
    const std::vector<std::string>& changed_entity_ids,
    const std::vector<std::string>& changed_inventory_ids,
    const std::vector<WorldEntityPatchDto>& manual_entity_patches) {
    WorldProjectionPatchDto patch;
    patch.new_projection_version = runtime.stateVersion();

    pathfinder::world_runtime::WorldProjectionAdapter adapter;
    auto cell_patches = adapter.buildCellPatches(changed_cell_ids, viewer_actor_key, runtime);
    if (cell_patches.is_ok()) patch.changed_cells = std::move(cell_patches.value());
    auto entity_patches = adapter.buildEntityPatches(changed_entity_ids, viewer_actor_key, runtime);
    if (entity_patches.is_ok()) patch.changed_entities = std::move(entity_patches.value());
    patch.changed_entities.insert(
        patch.changed_entities.end(), manual_entity_patches.begin(), manual_entity_patches.end());
    if (inventory_runtime) {
        pathfinder::world_inventory::InventoryProjectionAdapter inventory_adapter;
        auto inventory_patches = inventory_adapter.buildInventoryPatches(
            changed_inventory_ids, viewer_actor_key, *inventory_runtime);
        if (inventory_patches.is_ok()) patch.changed_inventories = std::move(inventory_patches.value());
    }
    return patch;
}

bool actorInventoryContains(
    const pathfinder::world_inventory::IInventoryRuntime& inventory_runtime,
    const std::string& actor_key,
    const std::string& entity_id,
    pathfinder::world_inventory::InventoryItemEntry* out_entry = nullptr) {
    auto inventory_res = inventory_runtime.findActorInventory(actor_key);
    if (inventory_res.is_error()) return false;
    for (const auto& entry : inventory_res.value()->entries) {
        if (entry.entity_id != entity_id) continue;
        if (out_entry) *out_entry = entry;
        return true;
    }
    return false;
}

WorldCommandExecutionResult executeObjectAction(
    WorldGridRuntime& runtime,
    const ContentRegistry& content_registry,
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime,
    WorldCommandContext& context,
    const WorldCommandDto& command,
    const std::string& action_key) {
    WorldCommandExecutionResult result;
    if (command.actor_key.empty() || command.target.target_entity_id.empty()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("object_action_missing_target");
        logObjectAction("failed action=" + action_key + " actor=" + command.actor_key + " target=" + command.target.target_entity_id + " reason=object_action_missing_target");
        return result;
    }

    auto actor_res = runtime.findActor(command.actor_key);
    if (actor_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back("object_action_target_unavailable");
        logObjectAction("blocked action=" + action_key + " actor=" + command.actor_key + " target=" + command.target.target_entity_id + " reason=object_action_target_unavailable");
        return result;
    }

    const auto actor = *actor_res.value();
    auto entity_res = runtime.findEntity(command.target.target_entity_id);
    WorldEntityInstance entity;
    bool target_in_inventory = false;
    pathfinder::world_inventory::InventoryItemEntry inventory_entry;
    if (entity_res.is_ok()) {
        entity = *entity_res.value();
        target_in_inventory = entity.location_kind == WorldEntityLocationKind::InInventory;
    } else if (inventory_runtime && action_key == "eat" && actorInventoryContains(
                   *inventory_runtime, command.actor_key, command.target.target_entity_id, &inventory_entry)) {
        entity.entity_id = inventory_entry.entity_id;
        entity.entity_key = inventory_entry.entity_key;
        entity.display_name_key = "object." + inventory_entry.entity_key + ".name";
        entity.quantity = inventory_entry.quantity;
        entity.stack_key = inventory_entry.stack_key;
        entity.stackable = inventory_entry.stackable;
        entity.state_keys = inventory_entry.state_keys;
        entity.numeric_states = inventory_entry.numeric_states;
        entity.location_kind = WorldEntityLocationKind::InInventory;
        target_in_inventory = true;
    } else {
        result.result_kind = WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back("object_action_target_unavailable");
        logObjectAction("blocked action=" + action_key + " actor=" + command.actor_key + " target=" + command.target.target_entity_id + " reason=object_action_target_unavailable");
        return result;
    }

    if (target_in_inventory) {
        if (action_key != "eat" || !inventory_runtime ||
            !actorInventoryContains(*inventory_runtime, command.actor_key, entity.entity_id, &inventory_entry)) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("object_action_target_unavailable");
            logObjectAction("blocked action=" + action_key + " actor=" + command.actor_key + " target=" + entity.entity_id + " object=" + entity.entity_key + " reason=inventory_target_unavailable");
            return result;
        }
    } else if (!entity.coord.has_value() || !sameOrAdjacent(actor.coord, *entity.coord)) {
        result.result_kind = WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back("object_action_out_of_range");
        logObjectAction("blocked action=" + action_key + " actor=" + command.actor_key + " target=" + entity.entity_id + " object=" + entity.entity_key + " reason=object_action_out_of_range");
        return result;
    }

    const auto target_key = command.target.target_item_key;
    const auto* feedback = bestFeedback(content_registry, entity.entity_key, action_key, target_key);
    const std::string effect_key = feedback ? feedback->effect_key : "no_visible_effect";
    const std::string feedback_key = feedback ? feedback->key.value() : entity.entity_key + "_" + action_key + "_no_visible_effect";
    const std::string reply_key = feedback ? feedback->reply_key : "effect.no_visible_effect.name";
    auto reply = content_registry.translate("zh_cn", reply_key);
    if (reply == reply_key) reply = "没有明显效果。";

    std::vector<std::string> changed_cell_ids;
    std::vector<std::string> changed_entity_ids;
    std::vector<std::string> changed_inventory_ids;
    std::vector<WorldEntityPatchDto> manual_entity_patches;

    if (action_key == "eat") {
        if (target_in_inventory) {
            pathfinder::world_inventory::InventoryTransferRequest consume;
            consume.transfer_id = command.command_id + "_consume_inventory";
            consume.transfer_kind = pathfinder::world_inventory::InventoryTransferKind::ConsumeToNowhere;
            consume.actor_key = command.actor_key;
            consume.entity_id = entity.entity_id;
            consume.entity_key = entity.entity_key;
            consume.quantity = 1;
            auto consume_res = inventory_runtime->transfer(consume);
            if (consume_res.is_error() || !consume_res.value().ok) {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back("object_action_consume_failed");
                logObjectAction("failed action=" + action_key + " actor=" + command.actor_key + " target=" + entity.entity_id + " object=" + entity.entity_key + " reason=inventory_consume_failed");
                return result;
            }
            for (const auto& id : consume_res.value().changed_inventory_ids) changed_inventory_ids.push_back(id);
            for (const auto& id : consume_res.value().changed_entity_ids) changed_entity_ids.push_back(id);
            for (const auto& id : consume_res.value().changed_cell_ids) changed_cell_ids.push_back(id);
        } else {
            const auto coord = *entity.coord;
            auto destroy_res = runtime.destroyEntity(entity.entity_id);
            if (destroy_res.is_error()) {
                result.result_kind = WorldCommandResultKind::Failed;
                result.failure_reason_keys.push_back("object_action_consume_failed");
                logObjectAction("failed action=" + action_key + " actor=" + command.actor_key + " target=" + entity.entity_id + " object=" + entity.entity_key + " reason=object_action_consume_failed");
                return result;
            }
            changed_cell_ids.push_back(coord.cellId());
            WorldEntityPatchDto removed;
            removed.entity_id = entity.entity_id;
            removed.op = PatchOp::Remove;
            removed.fields["entity_id"] = entity.entity_id;
            manual_entity_patches.push_back(std::move(removed));
        }
    }

    if (effect_key == "poison") {
        auto health_res = runtime.applyActorHealthDelta(command.actor_key, -1, {"object_action_poison", feedback_key});
        if (health_res.is_ok()) {
            for (const auto& id : health_res.value().changed_entity_ids) changed_entity_ids.push_back(id);
        }
    }

    if (effect_key == "restore_hunger") {
        auto hunger_res = runtime.applyActorNumericStateDelta(
            command.actor_key,
            "hunger",
            -30.0,
            0.0,
            100.0,
            {"object_action_restore_hunger", feedback_key});
        if (hunger_res.is_ok()) {
            for (const auto& id : hunger_res.value().changed_entity_ids) changed_entity_ids.push_back(id);
        }
    }

    result.result_kind = WorldCommandResultKind::Succeeded;

    WorldEventDto event;
    event.event_id = command.command_id + "_object_action";
    event.event_kind = action_key == "eat" ? "ObjectEaten" : "ObjectUsed";
    event.tick = context.currentTick();
    event.title_text = "尝试";
    event.body_text = command.actor_key + " 对 " + objectName(content_registry, entity.entity_key) + " 执行" + (action_key == "eat" ? "食用" : "使用") + "：" + reply;
    event.actor_key = command.actor_key;
    event.target_entity_id = entity.entity_id;
    if (entity.coord) event.coord = pathfinder::world_command::WorldCoordinateDto{entity.coord->x, entity.coord->y, entity.coord->layer_key};
    result.events.push_back(std::move(event));

    WorldExperienceDto exp;
    exp.experience_id = command.command_id + "_experience";
    exp.actor_key = command.actor_key;
    exp.command_key = action_key;
    exp.subject_entity_key = feedback && !feedback->subject_object_key.empty() ? feedback->subject_object_key : entity.entity_key;
    exp.target_entity_key = target_key;
    exp.effect_key = feedback && !feedback->knowledge_effect_key.empty() ? feedback->knowledge_effect_key : effect_key;
    exp.tick = context.currentTick();
    exp.reason_keys.push_back(feedback_key);
    result.experiences.push_back(std::move(exp));

    WorldStateDeltaDto delta;
    delta.delta_id = command.command_id + "_object_action_delta";
    delta.delta_kind = "object_action_attempted";
    delta.target_ref = entity.entity_id;
    delta.op = action_key == "eat" ? PatchOp::Remove : PatchOp::Update;
    delta.fields["object_key"] = entity.entity_key;
    delta.fields["action_key"] = action_key;
    delta.fields["effect_key"] = effect_key;
    if (effect_key == "restore_hunger") delta.fields["need_delta_hunger"] = "-30";
    result.state_deltas.push_back(std::move(delta));

    result.projection_patch_override = buildObjectActionPatch(
        runtime, inventory_runtime, command.actor_key, changed_cell_ids, changed_entity_ids, changed_inventory_ids, manual_entity_patches);
    logObjectAction("succeeded action=" + action_key + " actor=" + command.actor_key + " target=" + entity.entity_id + " object=" + entity.entity_key + " effect=" + effect_key + " feedback=" + feedback_key + " experiences=" + std::to_string(result.experiences.size()));
    return result;
}

class EatObjectCommandHandler final : public IWorldCommandHandler {
public:
    EatObjectCommandHandler(
        WorldGridRuntime& runtime,
        const ContentRegistry& content_registry,
        pathfinder::world_inventory::IInventoryRuntime* inventory_runtime)
        : runtime_(runtime), content_registry_(content_registry), inventory_runtime_(inventory_runtime) {}

    WorldCommandKind kind() const override { return WorldCommandKind::Eat; }

    Result<WorldCommandExecutionResult> execute(WorldCommandContext& context, const WorldCommandDto& command) const override {
        return Result<WorldCommandExecutionResult>::ok(executeObjectAction(runtime_, content_registry_, inventory_runtime_, context, command, "eat"));
    }

private:
    WorldGridRuntime& runtime_;
    const ContentRegistry& content_registry_;
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime_;
};

class UseObjectCommandHandler final : public pathfinder::world_modules::IWorldUseCommandHandler {
public:
    UseObjectCommandHandler(WorldGridRuntime& runtime, const ContentRegistry& content_registry)
        : runtime_(runtime), content_registry_(content_registry) {}

    bool supports(const WorldCommandDto& command) const override {
        return !command.target.target_entity_id.empty() && command.command_key != "set_actor_follow_player";
    }

    Result<WorldCommandExecutionResult> execute(WorldCommandContext& context, const WorldCommandDto& command) const override {
        return Result<WorldCommandExecutionResult>::ok(executeObjectAction(runtime_, content_registry_, nullptr, context, command, "use"));
    }

private:
    WorldGridRuntime& runtime_;
    const ContentRegistry& content_registry_;
};

} // namespace

std::shared_ptr<IWorldCommandHandler> createEatObjectCommandHandler(
    WorldGridRuntime& runtime,
    const ContentRegistry& content_registry,
    pathfinder::world_inventory::IInventoryRuntime* inventory_runtime) {
    return std::make_shared<EatObjectCommandHandler>(runtime, content_registry, inventory_runtime);
}

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createUseObjectCommandHandler(
    WorldGridRuntime& runtime,
    const ContentRegistry& content_registry) {
    return std::make_shared<UseObjectCommandHandler>(runtime, content_registry);
}

} // namespace pathfinder::world_object_interaction
