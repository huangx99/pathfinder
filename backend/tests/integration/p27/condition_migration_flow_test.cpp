#include "pathfinder/knowledge/knowledge_claim.h"
#include "pathfinder/condition/condition_expression_evaluator.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::knowledge;
using namespace pathfinder::condition;

static void test_knowledge_condition_normalizes_and_evaluates() {
    KnowledgeCondition legacy;
    legacy.condition_key = "state_decayed";
    auto normalized = normalizeKnowledgeCondition(legacy);
    assert(normalized.is_ok());
    assert(normalized.value().condition_key == "state_decayed");
    assert(normalized.value().canonical_condition_key == "condition:object_state:eq:decayed");
    assert(!normalized.value().condition_ref.empty());

    ConditionEvaluationContext context;
    context.context_type = "object";
    context.target = ObjectConditionView{};
    context.target->state_keys = {"decayed"};
    ConditionExpressionEvaluator evaluator;
    auto evaluated = evaluator.evaluate(normalized.value().condition_ref, context);
    assert(evaluated.is_ok());
    assert(evaluated.value().matched);
    std::cout << "knowledge_condition_normalizes_and_evaluates passed" << std::endl;
}

static void test_object_and_actor_legacy_fields_combine() {
    KnowledgeCondition legacy;
    legacy.object_state_keys = {"decayed"};
    legacy.actor_requirement_keys = {"has_tool"};
    auto normalized = normalizeKnowledgeCondition(legacy);
    assert(normalized.is_ok());
    assert(normalized.value().canonical_condition_key.rfind("condition:and:all:eq:", 0) == 0);

    ConditionEvaluationContext context;
    context.context_type = "object_actor";
    context.target = ObjectConditionView{};
    context.target->state_keys = {"decayed"};
    context.actor = ActorConditionView{};
    context.actor->requirement_keys = {"tool"};
    ConditionExpressionEvaluator evaluator;
    auto evaluated = evaluator.evaluate(normalized.value().condition_ref, context);
    assert(evaluated.is_ok());
    assert(evaluated.value().matched);
    std::cout << "object_and_actor_legacy_fields_combine passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "knowledge_condition") test_knowledge_condition_normalizes_and_evaluates();
    else if (mode == "combined_fields") test_object_and_actor_legacy_fields_combine();
    else {
        test_knowledge_condition_normalizes_and_evaluates();
        test_object_and_actor_legacy_fields_combine();
    }
    return 0;
}
