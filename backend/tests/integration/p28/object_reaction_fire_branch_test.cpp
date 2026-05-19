#include "pathfinder/reaction/reaction_fixtures.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using namespace pathfinder::reaction::fixtures;

static void test_fire_dry_branch_to_torch() {
    ObjectReactionResolver resolver;
    auto result = resolver.resolve(fireBranchInput(dryBranch()), coreP28Rules());
    assert(result.is_ok());
    const auto& value = result.value();
    assert(value.decision == ReactionDecision::Reacted);
    assert(value.selected_rule_key == "fire_dry_branch_to_torch");
    assert(value.object_changes.size() == 2);
    assert(value.object_changes[0].change_kind == ReactionOutputKind::TransformObject);
    assert(value.object_changes[0].to_definition_id.value() == "def_torch");
    assert(!value.feedbacks.empty());
    assert(value.feedbacks.front().safe_text.find("火把") != std::string::npos);
    assert(!value.knowledge_drafts.empty());
    assert(value.knowledge_drafts.front().effect_key == "transform_to_torch");
    assert(!value.knowledge_drafts.front().conditions.empty());
    std::cout << "fire_dry_branch_to_torch passed" << std::endl;
}

static void test_fire_wet_branch_condition_blocked() {
    ObjectReactionResolver resolver;
    auto result = resolver.resolve(fireBranchInput(wetBranch()), coreP28Rules());
    assert(result.is_ok());
    const auto& value = result.value();
    assert(value.decision == ReactionDecision::ConditionBlocked);
    assert(value.object_changes.empty());
    assert(!value.trace.condition_traces.empty());
    bool saw_dry_failure = false;
    for (const auto& trace : value.trace.condition_traces) {
        if (trace.expression_key == "condition:target_state:eq:dry" && !trace.matched) saw_dry_failure = true;
    }
    assert(saw_dry_failure);
    std::cout << "fire_wet_branch_condition_blocked passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "dry") test_fire_dry_branch_to_torch();
    else if (mode == "wet") test_fire_wet_branch_condition_blocked();
    else {
        test_fire_dry_branch_to_torch();
        test_fire_wet_branch_condition_blocked();
    }
    return 0;
}
