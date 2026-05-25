#include "pathfinder/world_modules/agent_wander/world_agent_wander_module.h"
#include <algorithm>
#include <functional>
#include <vector>

namespace pathfinder::world_agent_wander {
namespace {

bool isIdleHumanoidNpc(const pathfinder::world_runtime::WorldActorRuntime& actor,
                       const pathfinder::world_runtime::WorldGridRuntime& runtime,
                       const pathfinder::content::ContentRegistry& content_registry) {
    if (!actor.alive || actor.actor_key == "player") return false;
    auto entity_res = runtime.findEntity(actor.entity_id);
    if (entity_res.is_error()) return false;
    const auto* agent = content_registry.findAgent(entity_res.value()->entity_key);
    return agent && agent->embodiment == "humanoid";
}

std::vector<pathfinder::world_runtime::WorldCellCoord> wanderCandidates(
    const pathfinder::world_runtime::WorldActorRuntime& actor,
    uint64_t tick) {
    std::vector<pathfinder::world_runtime::WorldCellCoord> candidates = {
        {actor.coord.x + 1, actor.coord.y, actor.coord.layer_key},
        {actor.coord.x - 1, actor.coord.y, actor.coord.layer_key},
        {actor.coord.x, actor.coord.y + 1, actor.coord.layer_key},
        {actor.coord.x, actor.coord.y - 1, actor.coord.layer_key},
    };
    const auto rotate_by = static_cast<size_t>((std::hash<std::string>{}(actor.actor_key) + tick) % candidates.size());
    std::rotate(candidates.begin(), candidates.begin() + static_cast<long>(rotate_by), candidates.end());
    return candidates;
}

} // namespace

bool runAgentWanderTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    pathfinder::world_command::WorldCommandPipeline& pipeline,
    const std::function<bool(const std::string&)>& is_actor_busy,
    pathfinder::client_protocol::ClientCommandResponse& response) {
    using pathfinder::world_command::WorldCommandDto;
    using pathfinder::world_command::WorldCommandKind;
    using pathfinder::world_command::WorldCommandResultKind;
    using pathfinder::world_command::WorldCommandSource;
    using pathfinder::world_command::WorldCommandTargetKind;
    using pathfinder::world_command::WorldCoordinateDto;
    using pathfinder::world_command::WorldEventDto;

    if (response.result.result_kind != WorldCommandResultKind::Succeeded) return false;
    auto snap_res = runtime.snapshotForDebug();
    if (snap_res.is_error()) return false;

    bool moved_any = false;
    uint64_t command_version = response.new_projection_version;

    for (const auto& [actor_key, actor] : snap_res.value().actors) {
        if (!isIdleHumanoidNpc(actor, runtime, content_registry)) continue;
        if (is_actor_busy && is_actor_busy(actor_key)) continue;
        if (((std::hash<std::string>{}(actor_key) + command_version) % 2) != 0) continue;

        for (const auto& candidate : wanderCandidates(actor, command_version)) {
            WorldCommandDto move_command;
            move_command.command_id = response.result.command_id + "_wander_move_" + actor_key;
            move_command.command_kind = WorldCommandKind::Move;
            move_command.command_key = "move";
            move_command.source = WorldCommandSource::AgentDecision;
            move_command.actor_key = actor_key;
            move_command.target.target_kind = WorldCommandTargetKind::Coordinate;
            move_command.target.target_coord = WorldCoordinateDto{candidate.x, candidate.y, candidate.layer_key};
            move_command.context.issued_tick = command_version;
            move_command.context.parent_command_id = response.result.command_id;

            pipeline.setCurrentProjectionVersion(command_version);
            auto move_response_res = pipeline.execute("agent_wander_internal", move_command);
            if (move_response_res.is_error()) continue;
            auto move_response = move_response_res.value();
            if (move_response.result.result_kind != WorldCommandResultKind::Succeeded) continue;

            response.projection_patch.changed_cells.insert(
                response.projection_patch.changed_cells.end(),
                move_response.projection_patch.changed_cells.begin(),
                move_response.projection_patch.changed_cells.end());
            response.projection_patch.changed_entities.insert(
                response.projection_patch.changed_entities.end(),
                move_response.projection_patch.changed_entities.begin(),
                move_response.projection_patch.changed_entities.end());
            response.projection_patch.changed_inventories.insert(
                response.projection_patch.changed_inventories.end(),
                move_response.projection_patch.changed_inventories.begin(),
                move_response.projection_patch.changed_inventories.end());
            response.projection_patch.changed_knowledge.insert(
                response.projection_patch.changed_knowledge.end(),
                move_response.projection_patch.changed_knowledge.begin(),
                move_response.projection_patch.changed_knowledge.end());
            response.projection_patch.changed_area_effects.insert(
                response.projection_patch.changed_area_effects.end(),
                move_response.projection_patch.changed_area_effects.begin(),
                move_response.projection_patch.changed_area_effects.end());
            response.experiences.insert(response.experiences.end(), move_response.experiences.begin(), move_response.experiences.end());
            response.result.state_deltas.insert(
                response.result.state_deltas.end(),
                move_response.result.state_deltas.begin(),
                move_response.result.state_deltas.end());
            response.frontend_hints.insert(
                response.frontend_hints.end(),
                move_response.frontend_hints.begin(),
                move_response.frontend_hints.end());

            command_version = std::max(command_version, move_response.projection_patch.new_projection_version);
            response.new_projection_version = std::max(response.new_projection_version, command_version);
            response.projection_patch.new_projection_version = response.new_projection_version;

            WorldEventDto event;
            event.event_id = response.result.command_id + "_wander_" + actor_key;
            event.event_kind = "NpcWanderStep";
            event.tick = command_version;
            event.title_text = "NPC闲逛";
            event.body_text = "NPC在附近随意走动了一步。";
            event.actor_key = actor_key;
            event.coord = WorldCoordinateDto{candidate.x, candidate.y, candidate.layer_key};
            response.event_feed.push_back(std::move(event));
            moved_any = true;
            break;
        }
    }

    return moved_any;
}

} // namespace pathfinder::world_agent_wander
