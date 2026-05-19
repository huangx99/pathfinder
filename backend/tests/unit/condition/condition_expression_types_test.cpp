#include "pathfinder/condition/condition_expression_types.h"
#include <cassert>
#include <iostream>

using namespace pathfinder::condition;
using namespace pathfinder::foundation;

static void test_ref_validation() {
    ConditionExpressionRef empty;
    assert(empty.empty());
    assert(empty.validateBasic().is_error());

    ConditionExpressionRef ref;
    ref.inline_canonical_key = "condition:object_state:eq:decayed";
    assert(!ref.empty());
    assert(ref.validateBasic().is_ok());

    ref.inline_canonical_key = "hidden_truth";
    assert(ref.validateBasic().is_error());
    std::cout << "ref_validation passed" << std::endl;
}

static void test_value_debug_string() {
    ConditionExpressionValue value;
    assert(value.isEmpty());
    value.value = std::vector<std::string>{"wet", "dry"};
    assert(value.stableDebugString() == "[dry,wet]");
    std::cout << "value_debug_string passed" << std::endl;
}

static void test_definition_validation() {
    ConditionExpressionNode child;
    child.kind = ConditionExpressionNodeKind::HasState;
    child.field_path = "target.state";
    child.value.value = std::string("decayed");

    ConditionExpressionDefinition def;
    def.expression_id = ExpressionId("expr_decayed_state");
    def.context_type = "object";
    def.root = child;
    def.canonical_key = "condition:object_state:eq:decayed";
    def.summary_key = "condition.summary.object_state.decayed";
    assert(def.validateBasic().is_ok());
    std::cout << "definition_validation passed" << std::endl;
}

static void test_definition_rejects_forbidden_value() {
    ConditionExpressionNode child;
    child.kind = ConditionExpressionNodeKind::HasState;
    child.field_path = "target.state";
    child.value.value = std::string("hidden_truth");

    ConditionExpressionDefinition def;
    def.expression_id = ExpressionId("expr_forbidden_state");
    def.context_type = "object";
    def.root = child;
    def.canonical_key = "condition:object_state:eq:hidden_value";
    def.summary_key = "condition.summary.object_state.hidden_value";
    assert(def.validateBasic().is_error());

    child.value.value = std::vector<std::string>{"fresh", "raw_state"};
    def.root = child;
    assert(def.validateBasic().is_error());
    std::cout << "definition_rejects_forbidden_value passed" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "ref_validation") test_ref_validation();
    else if (mode == "value_debug_string") test_value_debug_string();
    else if (mode == "definition_validation") test_definition_validation();
    else if (mode == "definition_rejects_forbidden_value") test_definition_rejects_forbidden_value();
    else {
        test_ref_validation();
        test_value_debug_string();
        test_definition_validation();
        test_definition_rejects_forbidden_value();
    }
    return 0;
}
