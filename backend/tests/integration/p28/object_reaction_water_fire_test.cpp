#include "pathfinder/reaction/reaction_fixtures.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using namespace pathfinder::reaction::fixtures;

static void test_water_extinguishes_fire() {
    ObjectReactionResolver resolver;
    auto result = resolver.resolve(waterFireInput(), coreP28Rules());
    assert(result.is_ok());
    const auto& value = result.value();
    assert(value.decision == ReactionDecision::Reacted);
    assert(value.selected_rule_key == "water_extinguish_fire");
    assert(value.object_changes.size() == 2);
    assert(value.object_changes[0].change_kind == ReactionOutputKind::RemoveState);
    assert(value.object_changes[0].state_key == "burning");
    assert(value.object_changes[1].change_kind == ReactionOutputKind::ConsumeObject);
    assert(value.feedbacks.front().safe_text.find("浇灭") != std::string::npos);
    std::cout << "water_extinguishes_fire passed" << std::endl;
}

int main() {
    test_water_extinguishes_fire();
    return 0;
}
