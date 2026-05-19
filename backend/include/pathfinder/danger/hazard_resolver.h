#pragma once

#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/condition/condition_expression_registry.h"
#include "pathfinder/danger/danger_types.h"

namespace pathfinder::danger {

class CountermeasureEvaluator {
public:
    pathfinder::foundation::Result<CountermeasureAssessment> assess(
        const HazardExposureInput& input,
        const ThreatProfile& threat,
        const HazardRule& rule,
        const pathfinder::condition::ConditionExpressionRegistry* registry = nullptr) const;
};

class HazardResolver {
public:
    pathfinder::foundation::Result<HazardResolutionResult> resolve(
        const HazardExposureInput& input,
        const std::vector<HazardRule>& rules,
        const pathfinder::condition::ConditionExpressionRegistry* registry = nullptr) const;
};

pathfinder::condition::ConditionEvaluationContext buildDangerConditionContext(
    const HazardExposureInput& input,
    const ThreatProfile& threat);

} // namespace pathfinder::danger
