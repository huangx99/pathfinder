#include "pathfinder/reaction/reaction_fixtures.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::reaction;
using namespace pathfinder::reaction::fixtures;

static void test_resolver_no_matching_rule() {
    ObjectReactionResolver resolver;
    auto input = fireBranchInput(dryBranch());
    input.action_kind = ReactionActionKind::Touch;
    auto result = resolver.resolve(input, coreP28Rules());
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::NoMatchingRule);
    assert(!result.value().feedbacks.empty());
    std::cout << "resolver_no_matching_rule passed" << std::endl;
}

static void test_resolver_priority_stable_order() {
    auto low = fireDryBranchToTorchRule();
    low.rule_key = "low_priority_fire_rule";
    low.priority = 1;
    auto high = fireDryBranchToTorchRule();
    high.rule_key = "high_priority_fire_rule";
    high.priority = 10;
    ObjectReactionResolver resolver;
    auto result = resolver.resolve(fireBranchInput(dryBranch()), {low, high});
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);
    assert(result.value().selected_rule_key == "high_priority_fire_rule");
    std::cout << "resolver_priority_stable_order passed" << std::endl;
}

static void test_resolver_produce_object_draft() {
    auto rule = fireDryBranchToTorchRule();
    rule.rule_key = "fire_branch_produces_ember";
    ReactionOutputTemplate produce;
    produce.output_kind = ReactionOutputKind::ProduceObject;
    produce.target_role = ReactionObjectRole::Product;
    produce.product_definition_id = pathfinder::foundation::ObjectDefinitionId("def_ember");
    rule.output_templates = {produce};
    rule.knowledge_effect_key = "produce_ember";

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(fireBranchInput(dryBranch()), {rule});
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);
    assert(result.value().object_changes.size() == 1);
    assert(result.value().object_changes.front().change_kind == ReactionOutputKind::ProduceObject);
    assert(result.value().object_changes.front().role == ReactionObjectRole::Product);
    assert(result.value().object_changes.front().object_id.empty());
    assert(result.value().object_changes.front().to_definition_id.value() == "def_ember");
    std::cout << "resolver_produce_object_draft passed" << std::endl;
}

static void test_resolver_rule_feedback_text_is_data_driven() {
    auto rule = fireDryBranchToTorchRule();
    rule.rule_key = "fire_branch_custom_feedback";
    rule.feedback_key = "reaction.custom_feedback";
    rule.feedback_text = "这条反馈来自规则数据，不需要修改反应解析器。";

    ObjectReactionResolver resolver;
    auto result = resolver.resolve(fireBranchInput(dryBranch()), {rule});
    assert(result.is_ok());
    assert(result.value().decision == ReactionDecision::Reacted);
    assert(result.value().feedbacks.front().safe_text == rule.feedback_text);
    std::cout << "resolver_rule_feedback_text_is_data_driven passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "no_matching_rule") test_resolver_no_matching_rule();
    else if (mode == "priority") test_resolver_priority_stable_order();
    else if (mode == "produce_object_draft") test_resolver_produce_object_draft();
    else if (mode == "data_driven_feedback") test_resolver_rule_feedback_text_is_data_driven();
    else {
        test_resolver_no_matching_rule();
        test_resolver_priority_stable_order();
        test_resolver_produce_object_draft();
        test_resolver_rule_feedback_text_is_data_driven();
    }
    return 0;
}
