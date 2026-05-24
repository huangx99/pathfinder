#include "pathfinder/world_modules/bootstrap/default_playable_world_bootstrap.h"
#include "pathfinder/world_modules/core/world_module_utils.h"
#include <map>
#include <string>
#include <vector>

namespace pathfinder::world_modules {

pathfinder::foundation::Result<void> spawnDefaultPlayableWorldEntities(
    pathfinder::world_runtime::WorldGridRuntime& runtime,
    const pathfinder::content::ContentRegistry& content_registry,
    const pathfinder::world_runtime::WorldRuntimeConfig& config) {

    const auto spawn_object = [&](const std::string& entity_id,
                                  const std::string& object_key,
                                  const pathfinder::world_runtime::WorldCellCoord& preferred) {
        auto coord = nearestOpenCoord(runtime, preferred, 8);
        if (!coord) return;
        const auto* object = content_registry.findObject(object_key);
        const std::string display = object ? object->display_key : "object." + object_key + ".name";
        const auto tags = object ? object->safe_tags : std::vector<std::string>{};
        const auto numeric = object ? object->default_numeric : std::map<std::string, double>{};
        (void)runtime.spawnEntityOnMap(
            entity_id, object_key, display, *coord, 1, object_key + ":scenario", true, {}, numeric, tags);
    };

    if (auto coord = nearestOpenCoord(runtime, pathfinder::world_runtime::WorldCellCoord{1, 0, "surface"}, 8)) {
        (void)runtime.spawnActor(
            "companion",
            "companion",
            "agent.companion.name",
            *coord,
            config.default_vision_radius,
            false,
            {"actor", "humanoid", "npc", "prey"},
            {{"health", 8.0}, {"max_health", 8.0}});
    }

    spawn_object("scenario_camp_fire", "camp_fire", pathfinder::world_runtime::WorldCellCoord{2, 0, "surface"});

    if (auto coord = nearestOpenCoord(runtime, pathfinder::world_runtime::WorldCellCoord{4, 1, "surface"}, 8)) {
        const auto* beast = content_registry.findObject("beast_shadow");
        const auto tags = beast ? beast->safe_tags : std::vector<std::string>{"creature", "danger", "predator"};
        auto numeric = beast ? beast->default_numeric : std::map<std::string, double>{};
        if (numeric.find("health") == numeric.end()) numeric["health"] = 3.0;
        if (numeric.find("max_health") == numeric.end()) numeric["max_health"] = 3.0;
        if (numeric.find("attack_damage") == numeric.end()) numeric["attack_damage"] = 1.0;
        (void)runtime.spawnActor(
            "beast_shadow",
            "beast_shadow",
            beast ? beast->display_key : "object.beast_shadow.name",
            *coord,
            config.default_vision_radius,
            false,
            tags,
            numeric,
            "scenario_beast_shadow");
    }

    return pathfinder::foundation::Result<void>::ok();
}

} // namespace pathfinder::world_modules
