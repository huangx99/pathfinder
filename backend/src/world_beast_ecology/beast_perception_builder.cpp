#include "pathfinder/world_beast_ecology/beast_perception_builder.h"
#include "pathfinder/foundation/error.h"

namespace pathfinder::world_beast_ecology {

using pathfinder::foundation::Result;
using pathfinder::world_runtime::WorldActorRuntime;
using pathfinder::world_runtime::WorldEntityInstance;
using pathfinder::world_runtime::WorldResourceNodeRuntime;

Result<std::vector<BeastPerceptionItem>> BeastPerceptionBuilder::build(
    const std::string& actor_key,
    const BeastAgentProfile& profile,
    const WorldActorRuntime& actor,
    const std::vector<WorldActorRuntime>& nearby_actors,
    const std::vector<WorldEntityInstance>& nearby_entities,
    const std::vector<WorldResourceNodeRuntime>& nearby_resources,
    const std::vector<BeastPerceptionItem>& effects_and_areas) {

    std::vector<BeastPerceptionItem> items;
    items.reserve(nearby_actors.size() + nearby_entities.size() + nearby_resources.size() + effects_and_areas.size());

    // Build lookup from entity_id to tags so actors can inherit their entity tags
    std::map<std::string, std::vector<std::string>> entity_tags_by_id;
    for (const auto& entity : nearby_entities) {
        entity_tags_by_id[entity.entity_id] = entity.tag_keys;
    }

    for (const auto& other : nearby_actors) {
        if (other.actor_key == actor_key) continue;
        int dist = std::abs(other.coord.x - actor.coord.x) + std::abs(other.coord.y - actor.coord.y);
        if (dist > profile.vision_radius) continue;
        BeastPerceptionItem item;
        item.perception_id = "perception_actor_" + other.actor_key;
        item.kind = BeastPerceptionKind::Actor;
        item.target_ref = other.actor_key;
        item.target_key = other.entity_id;
        item.coord = other.coord;
        item.distance = dist;
        item.visible = true;
        auto it = entity_tags_by_id.find(other.entity_id);
        if (it != entity_tags_by_id.end()) {
            item.tag_keys = it->second;
        }
        items.push_back(item);
    }

    for (const auto& entity : nearby_entities) {
        if (!entity.coord.has_value()) continue;
        int dist = std::abs(entity.coord.value().x - actor.coord.x) + std::abs(entity.coord.value().y - actor.coord.y);
        if (dist > profile.vision_radius && dist > profile.hearing_radius) continue;
        BeastPerceptionItem item;
        item.perception_id = "perception_entity_" + entity.entity_id;
        item.kind = BeastPerceptionKind::Object;
        item.target_ref = entity.entity_id;
        item.target_key = entity.entity_key;
        item.coord = entity.coord;
        item.distance = dist;
        item.tag_keys = entity.tag_keys;
        item.visible = dist <= profile.vision_radius;
        items.push_back(item);
    }

    for (const auto& res : nearby_resources) {
        int dist = std::abs(res.coord.x - actor.coord.x) + std::abs(res.coord.y - actor.coord.y);
        if (dist > profile.vision_radius) continue;
        BeastPerceptionItem item;
        item.perception_id = "perception_resource_" + res.node_id;
        item.kind = BeastPerceptionKind::Resource;
        item.target_ref = res.node_id;
        item.target_key = res.resource_key;
        item.coord = res.coord;
        item.distance = dist;
        item.tag_keys = res.tag_keys;
        item.visible = true;
        items.push_back(item);
    }

    for (const auto& effect : effects_and_areas) {
        items.push_back(effect);
    }

    return Result<std::vector<BeastPerceptionItem>>::ok(std::move(items));
}

} // namespace pathfinder::world_beast_ecology
