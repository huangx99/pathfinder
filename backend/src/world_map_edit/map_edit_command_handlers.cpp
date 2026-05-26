#include "pathfinder/world_map_edit/map_edit_command_handlers.h"
#include "pathfinder/world_map_edit/map_edit_content_policy.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/logging/logger.h"
#include <algorithm>
#include <atomic>
#include <sstream>

namespace pathfinder::world_map_edit {
namespace {

using pathfinder::foundation::Result;
using pathfinder::world_command::IWorldCommandHandler;
using pathfinder::world_command::PatchOp;
using pathfinder::world_command::WorldCellPatchDto;
using pathfinder::world_command::WorldCommandContext;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandExecutionResult;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandResultKind;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_command::WorldEntityPatchDto;
using pathfinder::world_command::WorldEventDto;
using pathfinder::world_command::WorldProjectionPatchDto;
using pathfinder::world_runtime::WorldCellCoord;

std::atomic<uint64_t> g_spawn_counter{0};

std::string coordText(const WorldCellCoord& coord) {
    std::ostringstream oss;
    oss << coord.x << ',' << coord.y << ',' << coord.layer_key;
    return oss.str();
}

void logMapEdit(const std::string& message) {
    pathfinder::logging::log(pathfinder::logging::tag::Map, "map_edit " + message);
}

std::string makePlacedEntityId(const std::string& object_key, const WorldCellCoord& coord) {
    return "placed_" + object_key + "_" + coord.layer_key + "_" +
        std::to_string(coord.x) + "_" + std::to_string(coord.y) + "_" +
        std::to_string(++g_spawn_counter);
}

std::string makePlacedActorKey(const std::string& agent_key, const WorldCellCoord& coord) {
    return "placed_actor_" + agent_key + "_" + coord.layer_key + "_" +
        std::to_string(coord.x) + "_" + std::to_string(coord.y) + "_" +
        std::to_string(++g_spawn_counter);
}

bool containsKey(const std::vector<std::string>& values, const std::string& key) {
    return std::find(values.begin(), values.end(), key) != values.end();
}

const PaintableTerrainDefinition* findTerrain(const std::string& terrain_key) {
    static const auto terrains = buildPaintableTerrains();
    auto it = std::find_if(terrains.begin(), terrains.end(), [&](const auto& terrain) {
        return terrain.terrain_key == terrain_key;
    });
    return it == terrains.end() ? nullptr : &*it;
}

WorldProjectionPatchDto makePaintPatch(const WorldCellCoord& coord, const PaintableTerrainDefinition& terrain) {
    WorldProjectionPatchDto patch;
    WorldCellPatchDto cell;
    cell.cell_id = coord.cellId();
    cell.op = PatchOp::Update;
    cell.fields["x"] = std::to_string(coord.x);
    cell.fields["y"] = std::to_string(coord.y);
    cell.fields["layer_key"] = coord.layer_key;
    cell.fields["terrain_key"] = terrain.terrain_key;
    cell.fields["blocks_movement"] = terrain.blocks_movement ? "true" : "false";
    cell.fields["movement_cost"] = std::to_string(terrain.movement_cost);
    patch.changed_cells.push_back(std::move(cell));
    return patch;
}

class PaintTerrainCommandHandler final : public IWorldCommandHandler {
public:
    explicit PaintTerrainCommandHandler(pathfinder::world_runtime::IWorldRuntime& world_runtime)
        : world_runtime_(world_runtime) {}

    WorldCommandKind kind() const override { return WorldCommandKind::PaintTerrain; }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        if (command.command_key != "paint_terrain") {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("map_edit_wrong_paint_command_key");
            logMapEdit("paint blocked actor=" + command.actor_key + " key=" + command.command_key + " reason=map_edit_wrong_paint_command_key");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }
        if (!command.target.target_coord || command.target.target_item_key.empty()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("map_edit_missing_paint_target");
            logMapEdit("paint failed actor=" + command.actor_key + " reason=map_edit_missing_paint_target");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const auto terrain = findTerrain(command.target.target_item_key);
        if (!terrain) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("terrain_not_paintable");
            logMapEdit("paint blocked actor=" + command.actor_key + " terrain=" + command.target.target_item_key + " reason=terrain_not_paintable");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        WorldCellCoord coord{command.target.target_coord->x, command.target.target_coord->y, command.target.target_coord->layer_key};
        auto cell_res = world_runtime_.findCell(coord);
        if (cell_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("cell_not_found");
            logMapEdit("paint blocked actor=" + command.actor_key + " coord=" + coordText(coord) + " reason=cell_not_found");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const auto region_id = cell_res.value()->region_id;
        auto tag_keys = terrain->tag_keys;
        if (containsKey(cell_res.value()->tag_keys, "sandbox") && !containsKey(tag_keys, "sandbox")) {
            tag_keys.push_back("sandbox");
        }
        auto update_res = world_runtime_.createOrUpdateGeneratedCell(
            coord, terrain->terrain_key, region_id, terrain->blocks_movement, terrain->movement_cost, tag_keys);
        if (update_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("paint_terrain_failed");
            logMapEdit("paint failed actor=" + command.actor_key + " coord=" + coordText(coord) + " terrain=" + terrain->terrain_key + " reason=paint_terrain_failed");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;
        result.changed_cell_ids.push_back(coord.cellId());
        result.projection_patch_override = makePaintPatch(coord, *terrain);

        WorldEventDto event;
        event.event_id = command.command_id + "_paint";
        event.event_kind = "TerrainPainted";
        event.tick = context.currentTick();
        event.title_text = "绘制地形";
        event.body_text = "地形已变为" + terrain->display_name + "。";
        event.actor_key = command.actor_key;
        event.coord = command.target.target_coord;
        result.events.push_back(std::move(event));
        logMapEdit("paint succeeded actor=" + command.actor_key + " coord=" + coordText(coord) + " terrain=" + terrain->terrain_key);
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    pathfinder::world_runtime::IWorldRuntime& world_runtime_;
};

class SpawnEntityCommandHandler final : public IWorldCommandHandler {
public:
    SpawnEntityCommandHandler(
        pathfinder::world_runtime::IWorldRuntime& world_runtime,
        pathfinder::world_inventory::IWorldEntityLocationPort& location_port,
        const pathfinder::content::ContentRegistry& content_registry)
        : world_runtime_(world_runtime)
        , location_port_(location_port)
        , content_registry_(content_registry) {}

    WorldCommandKind kind() const override { return WorldCommandKind::SpawnEntity; }

    Result<WorldCommandExecutionResult> execute(
        WorldCommandContext& context,
        const WorldCommandDto& command) const override {
        WorldCommandExecutionResult result;
        if (!command.target.target_coord || command.target.target_item_key.empty()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("map_edit_missing_spawn_target");
            logMapEdit("spawn failed actor=" + command.actor_key + " reason=map_edit_missing_spawn_target");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        WorldCellCoord coord{command.target.target_coord->x, command.target.target_coord->y, command.target.target_coord->layer_key};
        auto cell_res = world_runtime_.findCell(coord);
        if (cell_res.is_error() || cell_res.value()->blocks_movement) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("spawn_cell_not_available");
            logMapEdit("spawn blocked actor=" + command.actor_key + " item=" + command.target.target_item_key + " coord=" + coordText(coord) + " reason=spawn_cell_not_available");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        if (command.command_key == "place_raw_object") {
            return executePlaceObject(context, command, coord);
        }
        if (command.command_key == "place_agent") {
            return executePlaceAgent(context, command, coord);
        }

        result.result_kind = WorldCommandResultKind::Blocked;
        result.failure_reason_keys.push_back("map_edit_wrong_spawn_command_key");
        logMapEdit("spawn blocked actor=" + command.actor_key + " key=" + command.command_key + " reason=map_edit_wrong_spawn_command_key");
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    Result<WorldCommandExecutionResult> executePlaceObject(
        WorldCommandContext& context,
        const WorldCommandDto& command,
        const WorldCellCoord& coord) const {
        WorldCommandExecutionResult result;
        const auto* object = content_registry_.findObject(command.target.target_item_key);
        if (!object || !isRawPlaceableObject(*object)) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("object_not_raw_placeable");
            logMapEdit("place object blocked actor=" + command.actor_key + " object=" + command.target.target_item_key + " coord=" + coordText(coord) + " reason=object_not_raw_placeable");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const auto entity_id = makePlacedEntityId(object->key.value(), coord);
        auto spawn_res = location_port_.spawnEntityOnMap(
            entity_id,
            object->key.value(),
            object->display_key,
            coord,
            std::max(1, object->default_quantity),
            object->key.value() + ":map_edit",
            true,
            {},
            object->default_numeric,
            object->safe_tags);
        if (spawn_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("spawn_entity_failed");
            logMapEdit("place object failed actor=" + command.actor_key + " object=" + object->key.value() + " entity_id=" + entity_id + " coord=" + coordText(coord) + " reason=spawn_entity_failed");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;
        result.changed_cell_ids.push_back(coord.cellId());
        result.changed_entity_ids.push_back(entity_id);

        WorldProjectionPatchDto patch;
        WorldEntityPatchDto entity;
        entity.entity_id = entity_id;
        entity.op = PatchOp::Add;
        entity.fields["entity_id"] = entity_id;
        entity.fields["entity_key"] = object->key.value();
        entity.fields["display_name_key"] = object->display_key;
        entity.fields["x"] = std::to_string(coord.x);
        entity.fields["y"] = std::to_string(coord.y);
        entity.fields["layer_key"] = coord.layer_key;
        entity.fields["location_kind"] = "on_map";
        entity.fields["visible"] = "true";
        entity.fields["blocks_movement"] = "false";
        patch.changed_entities.push_back(std::move(entity));
        result.projection_patch_override = std::move(patch);

        WorldEventDto event;
        event.event_id = command.command_id + "_spawn";
        event.event_kind = "EntitySpawnedByMapEdit";
        event.tick = context.currentTick();
        event.title_text = "投放原料";
        event.body_text = "投放了" + translateObjectName(content_registry_, object->key.value()) + "。";
        event.actor_key = command.actor_key;
        event.coord = command.target.target_coord;
        result.events.push_back(std::move(event));
        logMapEdit("place object succeeded actor=" + command.actor_key + " object=" + object->key.value() + " entity_id=" + entity_id + " coord=" + coordText(coord));
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    Result<WorldCommandExecutionResult> executePlaceAgent(
        WorldCommandContext& context,
        const WorldCommandDto& command,
        const WorldCellCoord& coord) const {
        WorldCommandExecutionResult result;
        const auto agents = buildPlaceableAgents(content_registry_);
        auto agent_it = std::find_if(agents.begin(), agents.end(), [&](const auto& agent) {
            return agent.agent_key == command.target.target_item_key;
        });
        if (agent_it == agents.end()) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("agent_not_placeable");
            logMapEdit("place agent blocked actor=" + command.actor_key + " agent=" + command.target.target_item_key + " coord=" + coordText(coord) + " reason=agent_not_placeable");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        const auto actor_key = makePlacedActorKey(agent_it->agent_key, coord);
        const auto entity_id = makePlacedEntityId(agent_it->agent_key, coord);
        auto spawn_res = world_runtime_.spawnActor(
            actor_key,
            agent_it->agent_key,
            agent_it->display_key,
            coord,
            agent_it->vision_radius,
            false,
            agent_it->tag_keys,
            agent_it->numeric_states,
            entity_id);
        if (spawn_res.is_error()) {
            result.result_kind = WorldCommandResultKind::Failed;
            result.failure_reason_keys.push_back("spawn_agent_failed");
            logMapEdit("place agent failed actor=" + command.actor_key + " agent=" + agent_it->agent_key + " actor_key=" + actor_key + " coord=" + coordText(coord) + " reason=spawn_agent_failed");
            return Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        result.result_kind = WorldCommandResultKind::Succeeded;
        result.changed_cell_ids.push_back(coord.cellId());
        result.changed_entity_ids.push_back(entity_id);

        WorldProjectionPatchDto patch;
        WorldEntityPatchDto entity;
        entity.entity_id = entity_id;
        entity.op = PatchOp::Add;
        entity.fields["entity_id"] = entity_id;
        entity.fields["entity_key"] = agent_it->agent_key;
        entity.fields["actor_key"] = actor_key;
        entity.fields["display_name_key"] = agent_it->display_key;
        entity.fields["x"] = std::to_string(coord.x);
        entity.fields["y"] = std::to_string(coord.y);
        entity.fields["layer_key"] = coord.layer_key;
        entity.fields["location_kind"] = "on_map";
        entity.fields["visible"] = "true";
        entity.fields["blocks_movement"] = "false";
        patch.changed_entities.push_back(std::move(entity));
        result.projection_patch_override = std::move(patch);

        WorldEventDto event;
        event.event_id = command.command_id + "_agent";
        event.event_kind = "AgentSpawnedByMapEdit";
        event.tick = context.currentTick();
        event.title_text = "投放小人";
        event.body_text = "投放了" + agent_it->display_name + "。";
        event.actor_key = command.actor_key;
        event.coord = command.target.target_coord;
        result.events.push_back(std::move(event));
        logMapEdit("place agent succeeded actor=" + command.actor_key + " agent=" + agent_it->agent_key + " spawned_actor=" + actor_key + " entity_id=" + entity_id + " coord=" + coordText(coord));
        return Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

    pathfinder::world_runtime::IWorldRuntime& world_runtime_;
    pathfinder::world_inventory::IWorldEntityLocationPort& location_port_;
    const pathfinder::content::ContentRegistry& content_registry_;
};

} // namespace

std::shared_ptr<IWorldCommandHandler> createPaintTerrainCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& world_runtime) {
    return std::make_shared<PaintTerrainCommandHandler>(world_runtime);
}

std::shared_ptr<IWorldCommandHandler> createSpawnEntityCommandHandler(
    pathfinder::world_runtime::IWorldRuntime& world_runtime,
    pathfinder::world_inventory::IWorldEntityLocationPort& location_port,
    const pathfinder::content::ContentRegistry& content_registry) {
    return std::make_shared<SpawnEntityCommandHandler>(world_runtime, location_port, content_registry);
}

} // namespace pathfinder::world_map_edit
