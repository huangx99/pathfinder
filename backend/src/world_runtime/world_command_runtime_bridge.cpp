#include "pathfinder/world_runtime/world_command_runtime_bridge.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>

namespace pathfinder::world_runtime {

using pathfinder::foundation::Result;
using pathfinder::foundation::ErrorDetail;
using namespace pathfinder::world_command;

namespace {

template<typename T>
T safeParse(const std::map<std::string, std::string>& params, const std::string& key, T default_value) {
    auto it = params.find(key);
    if (it == params.end()) return default_value;
    try {
        if constexpr (std::is_same_v<T, uint64_t>) {
            return static_cast<T>(std::stoull(it->second));
        } else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(it->second);
        }
    } catch (...) {
        return default_value;
    }
    return default_value;
}

} // anonymous namespace

WorldCommandRuntimeBridge::WorldCommandRuntimeBridge(IWorldRuntime& runtime)
    : runtime_(runtime) {}

Result<WorldCommandExecutionResult> WorldCommandRuntimeBridge::handleGenerateWorld(
    const WorldCommandContext& context,
    const WorldCommandDto& command) {

    WorldCommandExecutionResult result;
    result.result_kind = WorldCommandResultKind::Succeeded;

    WorldRuntimeConfig config;
    config.world_id = command.parameters.count("world_id") ? command.parameters.at("world_id") : "world_default";
    config.seed = safeParse<uint64_t>(command.parameters, "seed", 42);
    config.region_size = safeParse<int>(command.parameters, "region_size", 9);
    config.default_vision_radius = safeParse<int>(command.parameters, "vision_radius", 3);

    auto init_res = runtime_.initialize(config);
    if (init_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("world_init_failed");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    auto gen_res = runtime_.generateInitialWorld(config);
    if (gen_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("world_generation_failed");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    WorldEventDto event;
    event.event_id = command.command_id + "_gen";
    event.event_kind = "WorldGenerated";
    event.tick = context.currentTick();
    event.title_text = "世界生成";
    event.body_text = "世界已生成，你在 " + config.world_id + "。";
    event.actor_key = command.actor_key;
    result.events.push_back(std::move(event));

    // Build full projection patch for initial world
    auto snap_res = runtime_.snapshotForDebug();
    if (snap_res.is_ok()) {
        std::vector<std::string> all_cell_ids;
        for (const auto& [id, cell] : snap_res.value().cells) {
            all_cell_ids.push_back(id);
        }
        std::vector<std::string> all_entity_ids;
        for (const auto& [id, entity] : snap_res.value().entities) {
            all_entity_ids.push_back(id);
        }
        result.projection_patch_override = buildProjectionPatch(all_cell_ids, all_entity_ids, command.actor_key, true);
    }

    return Result<WorldCommandExecutionResult>::ok(std::move(result));
}

Result<WorldCommandExecutionResult> WorldCommandRuntimeBridge::handleMove(
    const WorldCommandContext& context,
    const WorldCommandDto& command) {

    WorldCommandExecutionResult result;

    if (!command.target.target_coord) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("missing_target_coord");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    WorldCellCoord target{
        command.target.target_coord->x,
        command.target.target_coord->y,
        command.target.target_coord->layer_key
    };

    auto move_res = runtime_.moveActor(command.actor_key, target);
    if (move_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("move_internal_error");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    auto move_result = move_res.value();
    if (!move_result.moved) {
        result.result_kind = WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back(toString(move_result.block_reason));

        WorldEventDto event;
        event.event_id = command.command_id + "_blocked";
        event.event_kind = "MoveBlocked";
        event.tick = context.currentTick();
        event.title_text = "移动受阻";
        event.body_text = "无法移动到目标位置: " + toString(move_result.block_reason);
        event.actor_key = command.actor_key;
        result.events.push_back(std::move(event));

        result.frontend_hints.push_back(WorldFrontendHintDto{});
        result.frontend_hints.back().hint_kind = FrontendHintKind::FloatingText;
        result.frontend_hints.back().target_coord = command.target.target_coord;
        result.frontend_hints.back().text = "无法移动";

        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    result.result_kind = WorldCommandResultKind::Succeeded;

    WorldEventDto event;
    event.event_id = command.command_id + "_moved";
    event.event_kind = "ActorMoved";
    event.tick = context.currentTick();
    event.title_text = "移动";
    event.body_text = "你移动到了新位置。";
    event.actor_key = command.actor_key;
    event.coord = command.target.target_coord;
    result.events.push_back(std::move(event));

    // State deltas for the move
    WorldStateDeltaDto delta;
    delta.delta_id = command.command_id + "_delta";
    delta.delta_kind = "actor_move";
    delta.target_ref = command.actor_key;
    delta.op = PatchOp::Update;
    delta.fields["x"] = std::to_string(target.x);
    delta.fields["y"] = std::to_string(target.y);
    delta.fields["layer_key"] = target.layer_key;
    result.state_deltas.push_back(std::move(delta));

    // Build projection patch with runtime changes
    result.projection_patch_override = buildProjectionPatch(
        move_result.changed_cell_ids,
        move_result.changed_entity_ids,
        command.actor_key,
        false);

    return Result<WorldCommandExecutionResult>::ok(std::move(result));
}

Result<WorldCommandExecutionResult> WorldCommandRuntimeBridge::handleInspect(
    const WorldCommandContext& context,
    const WorldCommandDto& command) {

    WorldCommandExecutionResult result;
    result.result_kind = WorldCommandResultKind::Succeeded;

    auto inspect_res = runtime_.inspect(command.actor_key, command.target);
    if (inspect_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("inspect_internal_error");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    auto inspect_result = inspect_res.value();

    WorldEventDto event;
    event.event_id = command.command_id + "_inspect";
    event.event_kind = "Inspect";
    event.tick = context.currentTick();
    event.title_text = "观察";
    event.actor_key = command.actor_key;

    if (inspect_result.found) {
        event.body_text = inspect_result.description_text;
        if (command.target.target_coord) {
            event.coord = command.target.target_coord;
        }
        if (!inspect_result.inspected_entity_id.empty()) {
            event.target_entity_id = inspect_result.inspected_entity_id;
        }
    } else {
        event.body_text = "你无法观察那个目标。";
    }

    result.events.push_back(std::move(event));
    return Result<WorldCommandExecutionResult>::ok(std::move(result));
}

Result<WorldCommandExecutionResult> WorldCommandRuntimeBridge::handleWait(
    const WorldCommandContext& context,
    const WorldCommandDto& command) {

    WorldCommandExecutionResult result;

    uint64_t tick_delta = safeParse<uint64_t>(command.parameters, "tick_delta", 1);

    auto wait_res = runtime_.advanceWorldTime(tick_delta);
    if (wait_res.is_error()) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("time_advance_failed");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    result.result_kind = WorldCommandResultKind::Succeeded;

    WorldEventDto event;
    event.event_id = command.command_id + "_wait";
    event.event_kind = "TimePassed";
    event.tick = context.currentTick();
    event.title_text = "时间流逝";
    event.body_text = "你等待了一会儿。";
    event.actor_key = command.actor_key;
    result.events.push_back(std::move(event));

    return Result<WorldCommandExecutionResult>::ok(std::move(result));
}


Result<WorldCommandExecutionResult> WorldCommandRuntimeBridge::handleAttack(
    const WorldCommandContext& context,
    const WorldCommandDto& command) {

    WorldCommandExecutionResult result;
    result.result_kind = WorldCommandResultKind::Blocked;

    if (command.target.target_actor_key.empty()) {
        result.failure_reason_keys.push_back("attack_missing_target_actor");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    auto attacker_res = runtime_.findActor(command.actor_key);
    auto target_res = runtime_.findActor(command.target.target_actor_key);
    if (attacker_res.is_error() || target_res.is_error()) {
        result.failure_reason_keys.push_back("attack_actor_not_found");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    const auto attacker = *attacker_res.value();
    const auto target = *target_res.value();
    if (!attacker.alive || !target.alive) {
        result.failure_reason_keys.push_back("attack_actor_not_alive");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }
    if (attacker.coord.layer_key != target.coord.layer_key ||
        std::abs(attacker.coord.x - target.coord.x) > 1 ||
        std::abs(attacker.coord.y - target.coord.y) > 1) {
        result.failure_reason_keys.push_back("attack_target_out_of_range");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    int damage = safeParse<int>(command.parameters, "damage", 1);
    damage = std::clamp(damage, 1, 1000);
    auto health_res = runtime_.applyActorHealthDelta(
        command.target.target_actor_key,
        -damage,
        {"attack", toString(command.source)});
    if (health_res.is_error() || !health_res.value().ok) {
        result.result_kind = WorldCommandResultKind::Failed;
        result.failure_reason_keys.push_back("attack_health_apply_failed");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    const auto health = health_res.value();
    result.result_kind = WorldCommandResultKind::Succeeded;
    result.changed_entity_ids = health.changed_entity_ids;

    WorldStateDeltaDto delta;
    delta.delta_id = command.command_id + "_health_delta";
    delta.delta_kind = "actor_health_changed";
    delta.target_ref = command.target.target_actor_key;
    delta.op = PatchOp::Update;
    delta.fields["previous_health"] = std::to_string(health.previous_health);
    delta.fields["health"] = std::to_string(health.new_health);
    delta.fields["max_health"] = std::to_string(health.max_health);
    delta.fields["alive"] = health.alive ? "true" : "false";
    delta.reason_keys = health.reason_keys;
    result.state_deltas.push_back(std::move(delta));

    WorldEventDto event;
    event.event_id = command.command_id + "_damaged";
    event.event_kind = health.alive ? "ActorDamaged" : "ActorDowned";
    event.tick = context.currentTick();
    event.title_text = health.alive ? "受到攻击" : "倒下";
    event.body_text = "野生威胁发动攻击，目标生命发生变化。";
    event.actor_key = command.actor_key;
    event.target_entity_id = health.entity_id;
    event.coord = WorldCoordinateDto{target.coord.x, target.coord.y, target.coord.layer_key};
    event.priority = 80;
    result.events.push_back(std::move(event));

    WorldExperienceDto exp;
    exp.experience_id = command.command_id + "_damage_taken_exp";
    exp.actor_key = command.target.target_actor_key;
    exp.command_key = "attack";
    exp.subject_entity_key = attacker.entity_id;
    exp.target_entity_key = target.entity_id;
    exp.effect_key = "damage_taken";
    exp.tick = context.currentTick();
    exp.reason_keys = {"attacked_by_wild_agent", toString(command.source)};
    result.experiences.push_back(std::move(exp));

    result.projection_patch_override = buildProjectionPatch(
        {},
        health.changed_entity_ids,
        command.actor_key,
        false);

    return Result<WorldCommandExecutionResult>::ok(std::move(result));
}

WorldProjectionPatchDto WorldCommandRuntimeBridge::buildProjectionPatch(
    const std::vector<std::string>& changed_cell_ids,
    const std::vector<std::string>& changed_entity_ids,
    const std::string& viewer_actor_key,
    bool requires_full_refresh) {

    WorldProjectionPatchDto patch;
    patch.new_projection_version = runtime_.stateVersion();
    patch.requires_full_refresh = requires_full_refresh;

    auto cell_patches = projection_adapter_.buildCellPatches(changed_cell_ids, viewer_actor_key, runtime_);
    if (cell_patches.is_ok()) {
        patch.changed_cells = std::move(cell_patches.value());
    }

    auto entity_patches = projection_adapter_.buildEntityPatches(changed_entity_ids, viewer_actor_key, runtime_);
    if (entity_patches.is_ok()) {
        patch.changed_entities = std::move(entity_patches.value());
    }

    return patch;
}

} // namespace pathfinder::world_runtime
