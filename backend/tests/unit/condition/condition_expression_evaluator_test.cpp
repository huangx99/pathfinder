#include "pathfinder/condition/condition_expression_evaluator.h"
#include <cassert>
#include <iostream>
#include <limits>

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

static void test_context_rejects_unsafe_numeric_fields() {
    ConditionEvaluationContext context;
    context.context_type = "civilization";
    context.civilization = CivilizationConditionView{};
    context.civilization->numeric_fields["hidden_truth_score"] = 1.0;
    assert(context.validateBasic().is_error());

    context.civilization->numeric_fields.clear();
    context.civilization->numeric_fields["known_edible_count"] = std::numeric_limits<double>::quiet_NaN();
    assert(context.validateBasic().is_error());

    context.civilization->numeric_fields.clear();
    context.civilization->numeric_fields["known_edible_count"] = std::numeric_limits<double>::infinity();
    assert(context.validateBasic().is_error());
    std::cout << "context_rejects_unsafe_numeric_fields passed" << std::endl;
}

static void test_canonical_rejects_unsupported_op() {
    ConditionEvaluationContext context;
    context.context_type = "object";
    context.target = ObjectConditionView{};
    context.target->state_keys = {"decayed"};

    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:object_state:neq:decayed";
    ConditionExpressionEvaluator evaluator;
    auto result = evaluator.evaluate(ref, context);
    assert(result.is_error());

    ref.inline_canonical_key = "condition:actor_requirement:eq:tool";
    context.actor = ActorConditionView{};
    context.actor->requirement_keys = {"tool"};
    result = evaluator.evaluate(ref, context);
    assert(result.is_error());
    std::cout << "canonical_rejects_unsupported_op passed" << std::endl;
}

static void test_source_target_domains() {
    ConditionEvaluationContext context;
    context.context_type = "reaction";
    context.source = ObjectConditionView{};
    context.source->state_keys = {"burning"};
    context.source->tag_keys = {"fire_source"};
    context.target = ObjectConditionView{};
    context.target->state_keys = {"dry"};
    context.target->tag_keys = {"combustible"};

    ConditionExpressionEvaluator evaluator;
    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:source_state:eq:burning";
    auto result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);

    ref.inline_canonical_key = "condition:target_state:eq:dry";
    result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);

    ref.inline_canonical_key = "condition:source_tag:has:fire_source";
    result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);

    ref.inline_canonical_key = "condition:target_tag:has:combustible";
    result = evaluator.evaluate(ref, context);
    assert(result.is_ok());
    assert(result.value().matched);
    std::cout << "source_target_domains passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "object_state_canonical") test_object_state_canonical();
    else if (mode == "civilization_numeric") test_civilization_numeric();
    else if (mode == "combined_and_condition") test_combined_and_condition();
    else if (mode == "context_rejects_unsafe_numeric_fields") test_context_rejects_unsafe_numeric_fields();
    else if (mode == "canonical_rejects_unsupported_op") test_canonical_rejects_unsupported_op();
    else if (mode == "source_target_domains") test_source_target_domains();
    else {
        test_object_state_canonical();
        test_civilization_numeric();
        test_combined_and_condition();
        test_context_rejects_unsafe_numeric_fields();
        test_canonical_rejects_unsupported_op();
        test_source_target_domains();
    }
    return 0;
}
