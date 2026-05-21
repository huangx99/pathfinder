#include "pathfinder/world_beast_ecology/beast_perception_builder.h"
#include "pathfinder/world_beast_ecology/world_beast_ecology_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::world_beast_ecology;
using namespace pathfinder::world_runtime;

void run_world_beast_ecology_perception_tests() {
    std::cout << "Running world_beast_ecology perception tests:" << std::endl;

    BeastPerceptionBuilder builder;

    BeastAgentProfile profile;
    profile.profile_key = "test";
    profile.temperament = BeastTemperamentKind::Predatory;
    profile.vision_radius = 3;
    profile.hearing_radius = 5;

    WorldActorRuntime actor;
    actor.actor_key = "beast_1";
    actor.coord = WorldCellCoord{0, 0, "surface"};

    // Nearby actor within vision
    WorldActorRuntime other_actor;
    other_actor.actor_key = "npc_1";
    other_actor.entity_id = "entity_npc_1";
    other_actor.coord = WorldCellCoord{1, 0, "surface"};

    // Entity outside vision but within hearing
    WorldEntityInstance entity;
    entity.entity_id = "entity_1";
    entity.entity_key = "generic_item";
    entity.coord = WorldCellCoord{0, 4, "surface"};
    entity.tag_keys.push_back("food_tag");

    // Effect/area perception (simulated)
    BeastPerceptionItem effect;
    effect.perception_id = "effect_1";
    effect.kind = BeastPerceptionKind::Effect;
    effect.target_ref = "area_fire_1";
    effect.target_key = "danger_effect_tag";
    effect.coord = WorldCellCoord{2, 2, "surface"};
    effect.distance = 4;
    effect.tag_keys.push_back("danger_effect_tag");
    effect.visible = true;

    std::vector<WorldActorRuntime> nearby_actors = {other_actor};
    std::vector<WorldEntityInstance> nearby_entities = {entity};
    std::vector<WorldResourceNodeRuntime> nearby_resources;
    std::vector<BeastPerceptionItem> effects = {effect};

    auto result = builder.build("beast_1", profile, actor, nearby_actors, nearby_entities, nearby_resources, effects);
    assert(result.is_ok());
    auto items = result.value();

    // Should have actor, entity, and effect
    assert(items.size() == 3);

    // Actor should be visible
    bool found_actor = false;
    bool found_entity = false;
    bool found_effect = false;

    for (const auto& item : items) {
        if (item.target_ref == "npc_1") {
            found_actor = true;
            assert(item.visible == true);
            assert(item.distance == 1);
        }
        if (item.target_ref == "entity_1") {
            found_entity = true;
            assert(item.visible == false); // Outside vision, inside hearing
            assert(item.distance == 4);
            assert(item.tag_keys.size() == 1);
            assert(item.tag_keys[0] == "food_tag");
        }
        if (item.target_ref == "area_fire_1") {
            found_effect = true;
            assert(item.kind == BeastPerceptionKind::Effect);
        }
    }

    assert(found_actor);
    assert(found_entity);
    assert(found_effect);

    // Invisible actor should not be returned if outside range
    WorldActorRuntime far_actor;
    far_actor.actor_key = "far_npc";
    far_actor.entity_id = "entity_far";
    far_actor.coord = WorldCellCoord{10, 10, "surface"};

    auto result2 = builder.build("beast_1", profile, actor, {far_actor}, {}, {}, {});
    assert(result2.is_ok());
    assert(result2.value().empty());

    std::cout << "All perception tests passed!" << std::endl;
}
