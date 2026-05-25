#include "pathfinder/v3_sandbox/v3_sandbox_runtime.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace pathfinder::v3_sandbox;

namespace {

V3SandboxRuntime makeRuntime() {
    return V3SandboxRuntime::createDefault(PATHFINDER_CONTENT_ROOT);
}

bool hasKnowledge(const V3SandboxSnapshot& snapshot, const std::string& object_key, const std::string& action_key, const std::string& effect_key) {
    for (const auto& agent : snapshot.agents) {
        for (const auto& claim : agent.knowledge) {
            if (claim.subject_object_key == object_key && claim.action_key == action_key && claim.effect_key == effect_key) return true;
        }
    }
    return false;
}

int inventoryQuantity(const V3SandboxSnapshot& snapshot, const std::string& object_key) {
    int total = 0;
    for (const auto& agent : snapshot.agents) {
        for (const auto& item : agent.inventory) {
            if (item.object_key == object_key) total += item.quantity;
        }
    }
    return total;
}

bool unlockedObject(const V3SandboxSnapshot& snapshot, const std::string& object_key) {
    for (const auto& key : snapshot.unlocked_objects) {
        if (key == object_key) return true;
    }
    return false;
}

void run_place_and_paint_commands() {
    auto runtime = makeRuntime();
    V3SandboxCommand paint;
    paint.kind = V3SandboxCommandKind::PaintTerrain;
    paint.x = 1;
    paint.y = 1;
    paint.terrain_key = "water";
    assert(runtime.submit(paint).accepted);

    V3SandboxCommand blocked_place;
    blocked_place.kind = V3SandboxCommandKind::PlaceObject;
    blocked_place.x = 1;
    blocked_place.y = 1;
    blocked_place.object_key = "red_berry";
    auto blocked = runtime.submit(blocked_place);
    assert(!blocked.accepted);
    assert(blocked.error == "cell_not_walkable");
}

void run_local_learning_loop() {
    auto runtime = makeRuntime();
    V3SandboxCommand place;
    place.kind = V3SandboxCommandKind::PlaceObject;
    place.x = 2;
    place.y = 2;
    place.object_key = "red_berry";
    assert(runtime.submit(place).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 2;
    agent.y = 3;
    agent.agent_name = "阿木";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 1;
    assert(runtime.submit(tick).accepted);

    const auto snapshot = runtime.snapshot();
    assert(runtime.knowledgeRepositorySize() == 1);
    assert(hasKnowledge(snapshot, "red_berry", "eat", "restore_hunger"));
    assert(unlockedObject(snapshot, "bitter_leaf"));
}

void run_rotten_berry_is_personal_experience_not_god_view() {
    auto runtime = makeRuntime();
    V3SandboxCommand rotten;
    rotten.kind = V3SandboxCommandKind::PlaceObject;
    rotten.x = 4;
    rotten.y = 4;
    rotten.object_key = "decayed_red_berry";
    assert(runtime.submit(rotten).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 4;
    agent.y = 5;
    agent.agent_name = "白";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 1;
    assert(runtime.submit(tick).accepted);

    auto snapshot = runtime.snapshot();
    assert(hasKnowledge(snapshot, "decayed_red_berry", "eat", "poison"));
    assert(!hasKnowledge(snapshot, "red_berry", "eat", "restore_hunger"));
    assert(!unlockedObject(snapshot, "bitter_leaf"));
    assert(!snapshot.agents.empty());
    assert(snapshot.agents.front().health < 100.0);
}

void run_no_feedback_becomes_knowledge() {
    auto runtime = makeRuntime();

    V3SandboxCommand first;
    first.kind = V3SandboxCommandKind::PlaceObject;
    first.x = 2;
    first.y = 2;
    first.object_key = "red_berry";
    assert(runtime.submit(first).accepted);

    V3SandboxCommand second = first;
    second.x = 3;
    second.y = 3;
    assert(runtime.submit(second).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 2;
    agent.y = 3;
    agent.agent_name = "试验者";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 2;
    assert(runtime.submit(tick).accepted);

    const auto snapshot = runtime.snapshot();
    assert(hasKnowledge(snapshot, "red_berry", "eat", "restore_hunger"));
    assert(hasKnowledge(snapshot, "red_berry", "use", "no_visible_effect"));
}

void run_agent_pickup_inventory() {
    auto runtime = makeRuntime();
    V3SandboxCommand stone;
    stone.kind = V3SandboxCommandKind::PlaceObject;
    stone.x = 3;
    stone.y = 3;
    stone.object_key = "stone_flake";
    assert(runtime.submit(stone).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 3;
    agent.y = 4;
    agent.agent_name = "背包小人";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 2;
    assert(runtime.submit(tick).accepted);

    const auto snapshot = runtime.snapshot();
    assert(hasKnowledge(snapshot, "stone_flake", "use", "use_hint"));
    assert(inventoryQuantity(snapshot, "stone_flake") == 1);
}

void run_perception_radius_blocks_god_view() {
    auto runtime = makeRuntime();
    V3SandboxCommand far_food;
    far_food.kind = V3SandboxCommandKind::PlaceObject;
    far_food.x = 8;
    far_food.y = 8;
    far_food.object_key = "red_berry";
    assert(runtime.submit(far_food).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 0;
    agent.y = 0;
    agent.agent_name = "近视小人";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 1;
    assert(runtime.submit(tick).accepted);

    const auto snapshot = runtime.snapshot();
    assert(!hasKnowledge(snapshot, "red_berry", "eat", "restore_hunger"));
}

void run_stone_hint_unlocks_wood() {
    auto runtime = makeRuntime();
    V3SandboxCommand stone;
    stone.kind = V3SandboxCommandKind::PlaceObject;
    stone.x = 3;
    stone.y = 3;
    stone.object_key = "stone_flake";
    assert(runtime.submit(stone).accepted);

    V3SandboxCommand agent;
    agent.kind = V3SandboxCommandKind::PlaceAgent;
    agent.x = 3;
    agent.y = 4;
    agent.agent_name = "石";
    assert(runtime.submit(agent).accepted);

    V3SandboxCommand tick;
    tick.kind = V3SandboxCommandKind::Tick;
    tick.tick_count = 1;
    assert(runtime.submit(tick).accepted);

    const auto snapshot = runtime.snapshot();
    assert(hasKnowledge(snapshot, "stone_flake", "use", "use_hint"));
    assert(unlockedObject(snapshot, "wood"));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    const std::string test = argv[1];
    if (test == "place_and_paint") run_place_and_paint_commands();
    else if (test == "local_learning") run_local_learning_loop();
    else if (test == "rotten_personal") run_rotten_berry_is_personal_experience_not_god_view();
    else if (test == "perception_no_god_view") run_perception_radius_blocks_god_view();
    else if (test == "stone_unlock") run_stone_hint_unlocks_wood();
    else if (test == "no_feedback_knowledge") run_no_feedback_becomes_knowledge();
    else if (test == "agent_pickup_inventory") run_agent_pickup_inventory();
    else return 1;
    std::cout << "ok " << test << '\n';
    return 0;
}
