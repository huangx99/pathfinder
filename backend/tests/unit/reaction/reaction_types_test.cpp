#include "pathfinder/reaction/reaction_fixtures.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace pathfinder::reaction;
using namespace pathfinder::reaction::fixtures;

static void test_enum_roundtrip() {
    assert(toString(ReactionActionKind::Combine) == "combine");
    assert(reactionActionKindFromString("combine").value() == ReactionActionKind::Combine);
    assert(toString(ReactionObjectRole::Source) == "source");
    assert(reactionObjectRoleFromString("source").value() == ReactionObjectRole::Source);
    assert(toString(ReactionOutputKind::TransformObject) == "transform_object");
    assert(reactionOutputKindFromString("transform_object").value() == ReactionOutputKind::TransformObject);
    assert(toString(ReactionDecision::ConditionBlocked) == "condition_blocked");
    assert(reactionDecisionFromString("condition_blocked").value() == ReactionDecision::ConditionBlocked);
    std::cout << "enum_roundtrip passed" << std::endl;
}

static void test_input_validation() {
    auto input = fireBranchInput(dryBranch());
    assert(input.validateBasic().is_ok());
    input.safe_context_keys = {"hidden_truth"};
    assert(input.validateBasic().is_error());
    input = fireBranchInput(dryBranch());
    input.objects.front().resource_values["fuel"] = std::numeric_limits<double>::quiet_NaN();
    assert(input.validateBasic().is_error());
    std::cout << "input_validation passed" << std::endl;
}

static void test_pattern_rejects_dynamic_tags() {
    auto rule = fireDryBranchToTorchRule();
    assert(rule.validateBasic().is_ok());
    rule.object_patterns[1].required_tag_keys.push_back("dry");
    assert(rule.validateBasic().is_error());
    rule = fireDryBranchToTorchRule();
    rule.object_patterns[1].forbidden_tag_keys.push_back("wet");
    assert(rule.validateBasic().is_error());
    std::cout << "pattern_rejects_dynamic_tags passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "enum_roundtrip") test_enum_roundtrip();
    else if (mode == "input_validation") test_input_validation();
    else if (mode == "pattern_rejects_dynamic_tags") test_pattern_rejects_dynamic_tags();
    else {
        test_enum_roundtrip();
        test_input_validation();
        test_pattern_rejects_dynamic_tags();
    }
    return 0;
}
