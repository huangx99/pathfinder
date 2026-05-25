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
