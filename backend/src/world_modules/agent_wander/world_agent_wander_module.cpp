#include "pathfinder/world_modules/agent_wander/world_agent_wander_module.h"
#include "pathfinder/logging/logger.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace pathfinder::world_agent_wander {
namespace {

using pathfinder::content::ContentRegistry;
using pathfinder::world_command::WorldCommandDto;
using pathfinder::world_command::WorldCommandKind;
using pathfinder::world_command::WorldCommandResultKind;
using pathfinder::world_command::WorldCommandSource;
using pathfinder::world_command::WorldCommandTargetKind;
using pathfinder::world_command::WorldCoordinateDto;
using pathfinder::world_command::WorldEventDto;
using pathfinder::world_runtime::WorldActorRuntime;
using pathfinder::world_runtime::WorldCellCoord;
using pathfinder::world_runtime::WorldEntityInstance;
using pathfinder::world_runtime::WorldEntityLocationKind;
using pathfinder::world_runtime::WorldGridRuntime;
using pathfinder::world_runtime::WorldRuntimeSnapshot;

std::string coordText(const WorldCellCoord& coord) {
    return std::to_string(coord.x) + "," + std::to_string(coord.y) + "," + coord.layer_key;
}

void logAgent(const std::string& message) {
    pathfinder::logging::log(pathfinder::logging::tag::Agent, message);
}

std::set<std::string>& attemptedObjectActions() {
    static std::set<std::string> attempts;
    return attempts;
}

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool hasAction(const std::vector<std::string>& actions, const std::string& action) {
    return std::find(actions.begin(), actions.end(), action) != actions.end();
}

bool sameOrAdjacent(const WorldCellCoord& left, const WorldCellCoord& right) {
    if (left.layer_key != right.layer_key) return false;
    return std::abs(left.x - right.x) <= 1 && std::abs(left.y - right.y) <= 1;
}

double actorHunger(const WorldRuntimeSnapshot& snapshot, const WorldActorRuntime& actor) {
    auto entity_it = snapshot.entities.find(actor.entity_id);
    if (entity_it == snapshot.entities.end()) return 0.0;
    auto hunger_it = entity_it->second.numeric_states.find("hunger");
    return hunger_it == entity_it->second.numeric_states.end() ? 0.0 : hunger_it->second;
}

bool isIdleHumanoidNpc(const WorldActorRuntime& actor,
                       const WorldGridRuntime& runtime,
                       const ContentRegistry& content_registry) {
    if (!actor.alive || actor.actor_key == "player") return false;
    auto entity_res = runtime.findEntity(actor.entity_id);
    if (entity_res.is_error()) return false;
    const auto* agent = content_registry.findAgent(entity_res.value()->entity_key);
    return agent && agent->embodiment == "humanoid";
}

bool isActorEntity(const WorldRuntimeSnapshot& snapshot, const std::string& entity_id) {
    for (const auto& [actor_key, actor] : snapshot.actors) {
        (void)actor_key;
        if (actor.entity_id == entity_id) return true;
    }
    return false;
}

bool canWanderTo(const WorldGridRuntime& runtime, const WorldCellCoord& coord) {
    auto cell_res = runtime.findCell(coord);
    if (cell_res.is_error()) return false;
    const auto* cell = cell_res.value();
    if (cell->blocks_movement) return false;
    return hasTag(cell->tag_keys, "sandbox");
}

std::vector<WorldCellCoord> wanderCandidates(const WorldActorRuntime& actor, uint64_t tick) {
    std::vector<WorldCellCoord> candidates = {
        {actor.coord.x + 1, actor.coord.y, actor.coord.layer_key},
        {actor.coord.x - 1, actor.coord.y, actor.coord.layer_key},
        {actor.coord.x, actor.coord.y + 1, actor.coord.layer_key},
        {actor.coord.x, actor.coord.y - 1, actor.coord.layer_key},
    };
    const auto rotate_by = static_cast<size_t>((std::hash<std::string>{}(actor.actor_key) + tick) % candidates.size());
    std::rotate(candidates.begin(), candidates.begin() + static_cast<long>(rotate_by), candidates.end());
    return candidates;
}

struct ObjectActionCandidate {
    std::string entity_id;
    std::string entity_key;
    WorldCommandKind command_kind{WorldCommandKind::Unknown};
    std::string action_key;
};

std::optional<ObjectActionCandidate> findNearbyObjectAction(
    const WorldRuntimeSnapshot& snapshot,
    const ContentRegistry& content_registry,
    const WorldActorRuntime& actor,
    uint64_t tick) {
    std::vector<const WorldEntityInstance*> candidates;
    for (const auto& [entity_id, entity] : snapshot.entities) {
        if (isActorEntity(snapshot, entity_id)) continue;
        if (!entity.coord.has_value()) continue;
        if (entity.location_kind != WorldEntityLocationKind::OnMap) continue;
        if (!sameOrAdjacent(actor.coord, *entity.coord)) continue;
        if (hasTag(entity.tag_keys, "creature") || hasTag(entity.tag_keys, "danger") || hasTag(entity.tag_keys, "predator")) continue;
        candidates.push_back(&entity);
    }
    if (candidates.empty()) return std::nullopt;

    std::sort(candidates.begin(), candidates.end(), [&](const auto* left, const auto* right) {
        const int ld = std::abs(left->coord->x - actor.coord.x) + std::abs(left->coord->y - actor.coord.y);
        const int rd = std::abs(right->coord->x - actor.coord.x) + std::abs(right->coord->y - actor.coord.y);
        if (ld != rd) return ld < rd;
        return left->entity_id < right->entity_id;
    });

    const auto rotate_by = static_cast<size_t>((std::hash<std::string>{}(actor.actor_key) + tick) % candidates.size());
    std::rotate(candidates.begin(), candidates.begin() + static_cast<long>(rotate_by), candidates.end());

    const bool hungry = actorHunger(snapshot, actor) >= 60.0;

    for (const auto* entity : candidates) {
        const auto* object = content_registry.findObject(entity->entity_key);
        if (!object) continue;
        const bool food_like = object->kind == "food" || hasTag(object->safe_tags, "food_like");
        std::vector<std::pair<WorldCommandKind, std::string>> actions;
        if (hungry && food_like && hasAction(object->allowed_actions, "eat")) actions.push_back({WorldCommandKind::Eat, "eat"});
        if (hasAction(object->allowed_actions, "use")) actions.push_back({WorldCommandKind::Use, "use"});
        actions.push_back({WorldCommandKind::Pickup, "pickup"});
        for (const auto& [kind, action] : actions) {
            const auto attempt_key = actor.actor_key + ":" + entity->entity_id + ":" + action;
            if (attemptedObjectActions().count(attempt_key) > 0) continue;
            logAgent("nearby object decision actor=" + actor.actor_key + " hunger=" + std::to_string(actorHunger(snapshot, actor)) + " entity=" + entity->entity_id + " object=" + entity->entity_key + " action=" + action + " food_like=" + (food_like ? std::string("true") : std::string("false")));
            return ObjectActionCandidate{entity->entity_id, entity->entity_key, kind, action};
        }
    }
    return std::nullopt;
}

void mergePipelineResponse(
    pathfinder::client_protocol::ClientCommandResponse& response,
    const pathfinder::world_command::WorldCommandResponseDto& inner) {
    response.projection_patch.changed_cells.insert(
        response.projection_patch.changed_cells.end(),
        inner.projection_patch.changed_cells.begin(), inner.projection_patch.changed_cells.end());
    response.projection_patch.changed_entities.insert(
        response.projection_patch.changed_entities.end(),
        inner.projection_patch.changed_entities.begin(), inner.projection_patch.changed_entities.end());
    response.projection_patch.changed_inventories.insert(
        response.projection_patch.changed_inventories.end(),
        inner.projection_patch.changed_inventories.begin(), inner.projection_patch.changed_inventories.end());
    response.projection_patch.changed_knowledge.insert(
        response.projection_patch.changed_knowledge.end(),
        inner.projection_patch.changed_knowledge.begin(), inner.projection_patch.changed_knowledge.end());
    response.projection_patch.changed_area_effects.insert(
        response.projection_patch.changed_area_effects.end(),
        inner.projection_patch.changed_area_effects.begin(), inner.projection_patch.changed_area_effects.end());
    response.event_feed.insert(response.event_feed.end(), inner.event_feed.begin(), inner.event_feed.end());
    response.experiences.insert(response.experiences.end(), inner.experiences.begin(), inner.experiences.end());
    response.result.state_deltas.insert(
        response.result.state_deltas.end(),
        inner.result.state_deltas.begin(), inner.result.state_deltas.end());
    response.frontend_hints.insert(
        response.frontend_hints.end(), inner.frontend_hints.begin(), inner.frontend_hints.end());
    response.new_projection_version = std::max(response.new_projection_version, inner.projection_patch.new_projection_version);
    response.projection_patch.new_projection_version = response.new_projection_version;
}

bool issueObjectAction(
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    const ObjectActionCandidate& candidate,
    const std::string& actor_key,
    pathfinder::client_protocol::ClientCommandResponse& response,
    uint64_t& command_version) {
    WorldCommandDto command;
    command.command_id = response.result.command_id + "_npc_object_" + actor_key;
    command.command_kind = candidate.command_kind;
    command.command_key = candidate.action_key;
    command.source = WorldCommandSource::AgentDecision;
    command.actor_key = actor_key;
    command.target.target_kind = WorldCommandTargetKind::Entity;
    command.target.target_entity_id = candidate.entity_id;
    command.context.issued_tick = command_version;
    command.context.parent_command_id = response.result.command_id;

    pipeline.setCurrentProjectionVersion(command_version);
    auto inner_res = pipeline.execute("agent_object_internal", command);
    if (inner_res.is_error()) {
        logAgent("object action pipeline error actor=" + actor_key + " entity=" + candidate.entity_id + " action=" + candidate.action_key);
        return false;
    }
    auto inner = inner_res.value();
    if (inner.result.result_kind != WorldCommandResultKind::Succeeded) {
        logAgent("object action not succeeded actor=" + actor_key + " entity=" + candidate.entity_id + " object=" + candidate.entity_key + " action=" + candidate.action_key + " result=" + pathfinder::world_command::toString(inner.result.result_kind));
        return false;
    }

    attemptedObjectActions().insert(actor_key + ":" + candidate.entity_id + ":" + candidate.action_key);
    mergePipelineResponse(response, inner);
    command_version = response.new_projection_version;
    logAgent("object action succeeded actor=" + actor_key + " entity=" + candidate.entity_id + " object=" + candidate.entity_key + " action=" + candidate.action_key + " new_version=" + std::to_string(command_version));
    return true;
}

bool issueWanderMove(
    const WorldActorRuntime& actor,
    const WorldCellCoord& candidate,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    pathfinder::client_protocol::ClientCommandResponse& response,
    uint64_t& command_version) {
    WorldCommandDto move_command;
    move_command.command_id = response.result.command_id + "_wander_move_" + actor.actor_key;
    move_command.command_kind = WorldCommandKind::Move;
    move_command.command_key = "move";
    move_command.source = WorldCommandSource::AgentDecision;
    move_command.actor_key = actor.actor_key;
    move_command.target.target_kind = WorldCommandTargetKind::Coordinate;
    move_command.target.target_coord = WorldCoordinateDto{candidate.x, candidate.y, candidate.layer_key};
    move_command.context.issued_tick = command_version;
    move_command.context.parent_command_id = response.result.command_id;

    pipeline.setCurrentProjectionVersion(command_version);
    auto move_response_res = pipeline.execute("agent_wander_internal", move_command);
    if (move_response_res.is_error()) {
        logAgent("wander move pipeline error actor=" + actor.actor_key + " target=" + coordText(candidate));
        return false;
    }
    auto move_response = move_response_res.value();
    if (move_response.result.result_kind != WorldCommandResultKind::Succeeded) {
        logAgent("wander move not succeeded actor=" + actor.actor_key + " from=" + coordText(actor.coord) + " target=" + coordText(candidate) + " result=" + pathfinder::world_command::toString(move_response.result.result_kind));
        return false;
    }

    mergePipelineResponse(response, move_response);
    command_version = response.new_projection_version;

    WorldEventDto event;
    event.event_id = response.result.command_id + "_wander_" + actor.actor_key;
    event.event_kind = "NpcWanderStep";
    event.tick = command_version;
    event.title_text = "NPC闲逛";
    event.body_text = "NPC在附近随意走动了一步。";
    event.actor_key = actor.actor_key;
    event.coord = WorldCoordinateDto{candidate.x, candidate.y, candidate.layer_key};
    response.event_feed.push_back(std::move(event));
    logAgent("wander move succeeded actor=" + actor.actor_key + " from=" + coordText(actor.coord) + " to=" + coordText(candidate) + " new_version=" + std::to_string(command_version));
    return true;
}

} // namespace

bool runAgentWanderTicks(
    WorldGridRuntime& runtime,
    const ContentRegistry& content_registry,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    const std::function<bool(const std::string&)>& is_actor_busy,
    pathfinder::client_protocol::ClientCommandResponse& response) {
    if (response.result.result_kind != WorldCommandResultKind::Succeeded) return false;
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return false;

    bool acted_any = false;
    uint64_t command_version = response.new_projection_version;
    const auto snapshot = snap_res.value();

    for (const auto& [actor_key, actor] : snapshot.actors) {
        if (!isIdleHumanoidNpc(actor, runtime, content_registry)) continue;
        if (is_actor_busy && is_actor_busy(actor_key)) {
            logAgent("skip busy actor=" + actor_key);
            continue;
        }

        auto object_action = findNearbyObjectAction(snapshot, content_registry, actor, command_version);
        if (object_action.has_value() && issueObjectAction(pipeline, object_action.value(), actor_key, response, command_version)) {
            acted_any = true;
            continue;
        }

        if (((std::hash<std::string>{}(actor_key) + command_version) % 2) != 0) continue;
        for (const auto& candidate : wanderCandidates(actor, command_version)) {
            if (!canWanderTo(runtime, candidate)) continue;
            if (issueWanderMove(actor, candidate, pipeline, response, command_version)) {
                acted_any = true;
                break;
            }
        }
    }

    return acted_any;
}

void clearAgentWanderRuntimeState() {
    attemptedObjectActions().clear();
}

} // namespace pathfinder::world_agent_wander
