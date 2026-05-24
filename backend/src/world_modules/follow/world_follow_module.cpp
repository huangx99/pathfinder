#include "pathfinder/world_modules/follow/world_follow_module.h"
#include "pathfinder/world_modules/core/world_module_utils.h"
#include "pathfinder/world_runtime/world_projection_adapter.h"
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace pathfinder::world_follow {
namespace {

std::set<std::string>& followingActorKeys() {
    static std::set<std::string> keys;
    return keys;
}

class FollowUseCommandHandler final : public pathfinder::world_modules::IWorldUseCommandHandler {
public:
    explicit FollowUseCommandHandler(pathfinder::world_runtime::WorldGridRuntime& runtime) : runtime_(runtime) {}

    bool supports(const pathfinder::world_command::WorldCommandDto& command) const override {
        return command.command_key == "set_actor_follow_player";
    }

    pathfinder::foundation::Result<pathfinder::world_command::WorldCommandExecutionResult> execute(
        pathfinder::world_command::WorldCommandContext& context,
        const pathfinder::world_command::WorldCommandDto& command) const override {
        using namespace pathfinder::world_command;
        WorldCommandExecutionResult result;
        const std::string follower_actor_key = command.target.target_actor_key;
        auto actor_res = runtime_.findActor(command.actor_key);
        auto follower_res = runtime_.findActor(follower_actor_key);
        if (follower_actor_key.empty() || follower_actor_key == command.actor_key ||
            actor_res.is_error() || follower_res.is_error() ||
            !pathfinder::world_modules::sameOrAdjacent8(actor_res.value()->coord, follower_res.value()->coord)) {
            result.result_kind = WorldCommandResultKind::Blocked;
            result.failure_reason_keys.push_back("npc_follow_not_available");
            return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
        }

        followingActorKeys().insert(follower_actor_key);
        result.result_kind = WorldCommandResultKind::Succeeded;
        WorldEventDto event;
        event.event_id = command.command_id + "_follow_enabled";
        event.event_kind = "NpcFollowEnabled";
        event.tick = context.currentTick();
        event.title_text = "NPC开始跟随";
        event.body_text = "NPC会在你移动拉开距离后跟上来。";
        event.actor_key = follower_actor_key;
        result.events.push_back(std::move(event));
        return pathfinder::foundation::Result<WorldCommandExecutionResult>::ok(std::move(result));
    }

private:
    pathfinder::world_runtime::WorldGridRuntime& runtime_;
};

} // namespace

std::shared_ptr<pathfinder::world_modules::IWorldUseCommandHandler> createFollowUseCommandHandler(
    pathfinder::world_runtime::WorldGridRuntime& runtime) {
    return std::make_shared<FollowUseCommandHandler>(runtime);
}

bool runFollowTicks(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const std::string& leader_actor_key,
    pathfinder::client_protocol::ClientCommandResponse& response,
    const std::function<bool(const std::string&)>& is_actor_busy) {
    if (response.result.result_kind != pathfinder::world_command::WorldCommandResultKind::Succeeded) return false;
    if (followingActorKeys().empty()) return false;

    auto leader_res = runtime.findActor(leader_actor_key);
    if (leader_res.is_error()) return false;
    const auto leader = *leader_res.value();

    std::set<std::string> changed_cells;
    std::set<std::string> changed_entities;

    for (const auto& follower_key : followingActorKeys()) {
        if (follower_key == leader_actor_key) continue;
        if (is_actor_busy && is_actor_busy(follower_key)) continue;
        auto follower_res = runtime.findActor(follower_key);
        if (follower_res.is_error()) continue;
        auto follower = *follower_res.value();
        if (pathfinder::world_modules::sameOrAdjacent8(follower.coord, leader.coord)) continue;

        std::vector<pathfinder::world_runtime::WorldCellCoord> candidates = {
            {follower.coord.x + 1, follower.coord.y, follower.coord.layer_key},
            {follower.coord.x - 1, follower.coord.y, follower.coord.layer_key},
            {follower.coord.x, follower.coord.y + 1, follower.coord.layer_key},
            {follower.coord.x, follower.coord.y - 1, follower.coord.layer_key},
        };
        std::sort(candidates.begin(), candidates.end(), [&](const auto& left, const auto& right) {
            return pathfinder::world_modules::manhattan(left, leader.coord) < pathfinder::world_modules::manhattan(right, leader.coord);
        });

        for (const auto& candidate : candidates) {
            if (pathfinder::world_modules::sameCoord(candidate, leader.coord)) continue;
            auto move_res = runtime.moveActor(follower_key, candidate);
            if (move_res.is_error() || !move_res.value().moved) continue;
            for (const auto& id : move_res.value().changed_cell_ids) changed_cells.insert(id);
            for (const auto& id : move_res.value().changed_entity_ids) changed_entities.insert(id);

            pathfinder::world_command::WorldEventDto event;
            event.event_id = response.result.command_id + "_follow_" + follower_key;
            event.event_kind = "NpcFollowStep";
            event.tick = response.new_projection_version;
            event.title_text = "NPC跟随";
            event.body_text = "NPC跟随你的移动靠近了一步。";
            event.actor_key = follower_key;
            response.event_feed.push_back(std::move(event));
            break;
        }
    }

    if (changed_cells.empty() && changed_entities.empty()) return false;

    pathfinder::world_runtime::WorldProjectionAdapter world_adapter;
    std::vector<std::string> cell_ids(changed_cells.begin(), changed_cells.end());
    std::vector<std::string> entity_ids(changed_entities.begin(), changed_entities.end());
    auto cell_patches = world_adapter.buildCellPatches(cell_ids, leader_actor_key, runtime);
    if (cell_patches.is_ok()) {
        response.projection_patch.changed_cells.insert(
            response.projection_patch.changed_cells.end(),
            cell_patches.value().begin(), cell_patches.value().end());
    }
    auto entity_patches = world_adapter.buildEntityPatches(entity_ids, leader_actor_key, runtime);
    if (entity_patches.is_ok()) {
        response.projection_patch.changed_entities.insert(
            response.projection_patch.changed_entities.end(),
            entity_patches.value().begin(), entity_patches.value().end());
    }
    return true;
}

void clearFollowingActors() {
    followingActorKeys().clear();
}

} // namespace pathfinder::world_follow
