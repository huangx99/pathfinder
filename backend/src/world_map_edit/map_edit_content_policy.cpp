#include "pathfinder/world_map_edit/map_edit_content_policy.h"
#include <algorithm>

namespace pathfinder::world_map_edit {
namespace {

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

} // namespace

std::string translateObjectName(
    const pathfinder::content::ContentRegistry& content_registry,
    const std::string& object_key) {
    const auto key = "object." + object_key + ".name";
    auto translated = content_registry.translate("zh_cn", key);
    return translated == key ? object_key : translated;
}

std::string translateAgentName(
    const pathfinder::content::ContentRegistry& content_registry,
    const std::string& agent_key) {
    const auto* agent = content_registry.findAgent(agent_key);
    const auto key = agent && !agent->display_key.empty() ? agent->display_key : "agent." + agent_key + ".name";
    auto translated = content_registry.translate("zh_cn", key);
    return translated == key ? agent_key : translated;
}

bool isRawPlaceableObject(const pathfinder::content::ObjectDefinitionContent& object) {
    if (hasTag(object.safe_tags, "crafted") || hasTag(object.safe_tags, "manufactured")) return false;
    if (object.kind == "tool" || object.kind == "weapon") return false;
    return object.kind == "food" || object.kind == "material" || hasTag(object.safe_tags, "plant") || hasTag(object.safe_tags, "stone");
}

std::vector<PlaceableObjectDefinition> buildRawPlaceableObjects(
    const pathfinder::content::ContentRegistry& content_registry) {
    std::vector<PlaceableObjectDefinition> result;
    for (const auto* object : content_registry.allObjects()) {
        if (!object || !isRawPlaceableObject(*object)) continue;
        PlaceableObjectDefinition def;
        def.object_key = object->key.value();
        def.display_name = translateObjectName(content_registry, def.object_key);
        def.quantity = std::max(1, object->default_quantity);
        def.numeric_states = object->default_numeric;
        def.tag_keys = object->safe_tags;
        result.push_back(std::move(def));
    }
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.object_key < rhs.object_key;
    });
    return result;
}

std::vector<PlaceableAgentDefinition> buildPlaceableAgents(
    const pathfinder::content::ContentRegistry& content_registry) {
    std::vector<PlaceableAgentDefinition> result;
    for (const auto* agent : content_registry.allAgents()) {
        if (!agent || agent->embodiment != "humanoid") continue;
        PlaceableAgentDefinition def;
        def.agent_key = agent->key.value();
        def.display_name = translateAgentName(content_registry, def.agent_key);
        def.display_key = agent->display_key;
        def.embodiment = agent->embodiment;
        def.cognition_band = agent->cognition_band;
        def.vision_radius = 3;
        def.numeric_states["fear"] = agent->default_fear;
        def.numeric_states["hunger"] = agent->default_hunger;
        def.numeric_states["trust"] = agent->default_trust;
        def.tag_keys = {"actor", "npc", agent->embodiment, agent->cognition_band};
        if (agent->can_learn) def.tag_keys.push_back("can_learn");
        if (agent->can_teach) def.tag_keys.push_back("can_teach");
        result.push_back(std::move(def));
    }
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.agent_key < rhs.agent_key;
    });
    return result;
}

std::vector<PaintableTerrainDefinition> buildPaintableTerrains() {
    return {
        {"plain", "平地", false, 1, {"plain"}},
        {"forest", "树林", false, 2, {"forest"}},
        {"stone_field", "石地", false, 2, {"stone_field", "rough"}},
        {"water_edge", "浅水", true, 3, {"water_edge", "wet"}},
        {"blocked", "阻挡", true, 99, {"blocked", "rough"}}
    };
}

} // namespace pathfinder::world_map_edit
