#pragma once

#include "pathfinder/content/content_registry.h"
#include "pathfinder/content/content_types.h"
#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/knowledge/knowledge_types.h"
#include "pathfinder/world_runtime/iworld_runtime.h"
#include "pathfinder/world_runtime/world_runtime_types.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pathfinder::world_modules {

pathfinder::knowledge::KnowledgeOwner actorKnowledgeOwner(const std::string& actor_key);
int recipeInputQuantity(const pathfinder::content::ReactionInputDto& input);
std::string objectName(const pathfinder::content::ContentRegistry& registry, const std::string& object_key);
std::string joinStrings(const std::vector<std::string>& values);
std::string numericStatesParam(const std::map<std::string, double>& values);
bool hasTag(const std::vector<std::string>& tags, const std::string& tag);
int numericAsInt(const std::map<std::string, double>& values, const std::string& key, int fallback);
bool hasState(const std::vector<std::string>& states, const std::string& state);
int manhattan(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b);
bool sameOrAdjacent8(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b);
bool sameCoord(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b);
std::optional<pathfinder::world_runtime::WorldCellCoord> nearestOpenCoord(
    const pathfinder::world_runtime::IWorldRuntime& runtime,
    const pathfinder::world_runtime::WorldCellCoord& preferred,
    int radius = 6);

} // namespace pathfinder::world_modules
