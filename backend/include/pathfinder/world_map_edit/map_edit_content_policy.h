#pragma once

#include "pathfinder/content/content_registry.h"
#include <map>
#include <string>
#include <vector>

namespace pathfinder::world_map_edit {

struct PlaceableObjectDefinition {
    std::string object_key;
    std::string display_name;
    int quantity = 1;
    std::vector<std::string> state_keys;
    std::map<std::string, double> numeric_states;
    std::vector<std::string> tag_keys;
};

struct PaintableTerrainDefinition {
    std::string terrain_key;
    std::string display_name;
    bool blocks_movement = false;
    int movement_cost = 1;
    std::vector<std::string> tag_keys;
};

bool isRawPlaceableObject(const pathfinder::content::ObjectDefinitionContent& object);
std::vector<PlaceableObjectDefinition> buildRawPlaceableObjects(
    const pathfinder::content::ContentRegistry& content_registry);
std::vector<PaintableTerrainDefinition> buildPaintableTerrains();
std::string translateObjectName(
    const pathfinder::content::ContentRegistry& content_registry,
    const std::string& object_key);

} // namespace pathfinder::world_map_edit
