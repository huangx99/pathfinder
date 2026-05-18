#pragma once

#include "pathfinder/condition/condition_expression_context.h"
#include "pathfinder/condition/condition_expression_registry.h"
#include <optional>

namespace pathfinder::condition {

struct ConditionEvaluationTrace {
    std::string expression_key;
    bool matched{false};
    double score{0.0};
    std::vector<std::string> steps;
    std::vector<std::string> reason_keys;

    pathfinder::foundation::Result<void> validateBasic() const;
};

struct ConditionEvaluationResult {
    bool matched{false};
    double score{0.0};
    std::string canonical_condition_key;
    std::string summary_key;
    ConditionEvaluationTrace trace;

    pathfinder::foundation::Result<void> validateBasic() const;
};

class ConditionExpressionEvaluator {
public:
    pathfinder::foundation::Result<ConditionEvaluationResult> evaluate(
        const ConditionExpressionRef& ref,
        const ConditionEvaluationContext& context,
        const ConditionExpressionRegistry* registry = nullptr) const;

    pathfinder::foundation::Result<ConditionEvaluationResult> evaluateDefinition(
        const ConditionExpressionDefinition& definition,
        const ConditionEvaluationContext& context) const;
};

} // namespace pathfinder::condition
