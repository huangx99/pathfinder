#include "pathfinder/condition/condition_expression_evaluator.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::condition;

static void test_object_state_canonical() {
    ConditionEvaluationContext context;
    context.context_type = "object";
    context.target = ObjectConditionView{};
    context.target->state_keys = {"fresh", "decayed"};

    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:object_state:eq:decayed";
    ConditionExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);
    assert(result.value().score == 1.0);
    std::cout << "object_state_canonical passed" << std::endl;
}

static void test_civilization_numeric() {
    ConditionEvaluationContext context;
    context.context_type = "civilization";
    context.civilization = CivilizationConditionView{};
    context.civilization->numeric_fields["known_edible_count"] = 3.0;

    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:civilization:known_edible_count:gte:2";
    ConditionExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);
    assert(result.value().score == 1.0);
    std::cout << "civilization_numeric passed" << std::endl;
}

static void test_combined_and_condition() {
    ConditionEvaluationContext context;
    context.context_type = "object_actor";
    context.target = ObjectConditionView{};
    context.target->state_keys = {"decayed"};
    context.actor = ActorConditionView{};
    context.actor->requirement_keys = {"tool"};

    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:and:all:eq:condition:actor_requirement:has:tool+condition:object_state:eq:decayed";
    ConditionExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);
    assert(result.value().score == 1.0);
    std::cout << "combined_and_condition passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "object_state_canonical") test_object_state_canonical();
    else if (mode == "civilization_numeric") test_civilization_numeric();
    else if (mode == "combined_and_condition") test_combined_and_condition();
    else {
        test_object_state_canonical();
        test_civilization_numeric();
        test_combined_and_condition();
    }
    return 0;
}
