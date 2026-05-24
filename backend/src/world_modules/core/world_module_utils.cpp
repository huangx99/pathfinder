#include "pathfinder/world_modules/core/world_module_utils.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::world_modules {

pathfinder::knowledge::KnowledgeOwner actorKnowledgeOwner(const std::string& actor_key) {
    pathfinder::knowledge::KnowledgeOwner owner;
    owner.kind = pathfinder::knowledge::KnowledgeOwnerKind::Actor;
    owner.entity_id = pathfinder::foundation::EntityId(actor_key);
    return owner;
}

int recipeInputQuantity(const pathfinder::content::ReactionInputDto& input) {
    return input.min > 0 ? input.min : (input.quantity > 0 ? input.quantity : 1);
}

std::string objectName(const pathfinder::content::ContentRegistry& registry, const std::string& object_key) {
    const auto key = "object." + object_key + ".name";
    auto translated = registry.translate("zh_cn", key);
    return translated == key ? object_key : translated;
}

std::string joinStrings(const std::vector<std::string>& values) {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    return oss.str();
}

std::string numericStatesParam(const std::map<std::string, double>& values) {
    std::ostringstream oss;
    size_t index = 0;
    for (const auto& [key, value] : values) {
        if (index++ > 0) oss << ",";
        oss << key << "=" << value;
    }
    return oss.str();
}

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) {
    return std::find(tags.begin(), tags.end(), tag) != tags.end();
}

int numericAsInt(const std::map<std::string, double>& values, const std::string& key, int fallback) {
    auto it = values.find(key);
    if (it == values.end()) return fallback;
    return std::max(0, static_cast<int>(std::round(it->second)));
}

bool hasState(const std::vector<std::string>& states, const std::string& state) {
    return state.empty() || std::find(states.begin(), states.end(), state) != states.end();
}

int manhattan(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b) {
    if (a.layer_key != b.layer_key) return 1000000;
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool sameOrAdjacent8(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b) {
    return a.layer_key == b.layer_key && std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1;
}

bool sameCoord(const pathfinder::world_runtime::WorldCellCoord& a, const pathfinder::world_runtime::WorldCellCoord& b) {
    return a.layer_key == b.layer_key && a.x == b.x && a.y == b.y;
}

std::optional<pathfinder::world_runtime::WorldCellCoord> nearestOpenCoord(
    const pathfinder::world_runtime::IWorldRuntime& runtime,
    const pathfinder::world_runtime::WorldCellCoord& preferred,
    int radius) {
    for (int r = 0; r <= radius; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) + std::abs(dy) != r) continue;
                pathfinder::world_runtime::WorldCellCoord coord{preferred.x + dx, preferred.y + dy, preferred.layer_key};
                auto cell_res = runtime.findCell(coord);
                if (cell_res.is_error()) continue;
                if (cell_res.value()->blocks_movement) continue;
                return coord;
            }
        }
    }
    return std::nullopt;
}

} // namespace pathfinder::world_modules
