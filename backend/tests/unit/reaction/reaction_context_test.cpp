#include "pathfinder/reaction/reaction_fixtures.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using namespace pathfinder::reaction::fixtures;

static void test_context_builder_maps_source_and_target() {
    ReactionConditionContextBuilder builder;
    auto context = builder.build(fireBranchInput(dryBranch()));
    assert(context.is_ok());
    assert(context.value().source.has_value());
    assert(context.value().target.has_value());
    assert(context.value().source->state_keys == std::vector<std::string>{"burning"});
    assert(context.value().target->state_keys == std::vector<std::string>{"dry"});
    std::cout << "context_builder_maps_source_and_target passed" << std::endl;
}

int main() {
    test_context_builder_maps_source_and_target();
    return 0;
}
