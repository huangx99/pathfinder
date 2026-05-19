#include "pathfinder/agent_reasoning/reaction_planning_adapter.h"
#include "pathfinder/foundation/error.h"
#include "pathfinder/reaction/reaction_fixtures.h"
#include <algorithm>

namespace pathfinder::agent_reasoning {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;
using pathfinder::reaction::ReactionObjectRole;

namespace {

PlanPrecondition precondition(std::string domain, std::string key, std::string required, bool can_plan) {
    PlanPrecondition item;
    item.condition_id = "condition." + domain + "." + key + "." + required;
    item.condition_expression = "condition:test:eq:" + domain + "." + key + "." + required;
    item.missing_domain = std::move(domain);
    item.missing_key = std::move(key);
    item.required_value = std::move(required);
    item.can_be_planned = can_plan;
    return item;
}

PlanPrecondition preconditionFromReactionExecution(const pathfinder::reaction::ReactionExecutionPrecondition& value) {
    return precondition(value.missing_domain, value.missing_key, value.required_value, value.can_be_planned);
}

bool reactionRuleSupportsEffect(const pathfinder::reaction::ObjectReactionRule& rule, const std::string& effect_key) {
    if (rule.execution_effect_key == effect_key) return true;
    if (rule.knowledge_effect_key == effect_key) return true;
    return false;
}

std::vector<PlanPrecondition> dedupe(std::vector<PlanPrecondition> values) {
    std::vector<PlanPrecondition> deduped;
    for (const auto& value : values) {
        const auto duplicate = std::find_if(deduped.begin(), deduped.end(), [&](const auto& item) {
            return item.missing_domain == value.missing_domain &&
                   item.missing_key == value.missing_key &&
                   item.required_value == value.required_value;
        });
        if (duplicate == deduped.end()) deduped.push_back(value);
    }
    return deduped;
}

} // namespace

Result<std::vector<PlanPrecondition>> ReactionPlanningAdapter::preconditionsForEffect(
    const std::string& effect_key,
    const std::vector<pathfinder::reaction::ObjectReactionRule>& rules) const {
    if (effect_key.empty()) return Result<std::vector<PlanPrecondition>>::fail(makeError(ErrorCode::validation_failed, "reaction planning effect key empty"));
    std::vector<PlanPrecondition> resolved;
    for (const auto& rule : rules) {
        auto valid = rule.validateBasic();
        if (valid.is_error()) return Result<std::vector<PlanPrecondition>>::fail(valid.errors());
        if (!reactionRuleSupportsEffect(rule, effect_key)) continue;
        for (const auto& pattern : rule.object_patterns) {
            for (const auto& precondition_value : pattern.execution_preconditions) {
                resolved.push_back(preconditionFromReactionExecution(precondition_value));
            }
        }
        for (const auto& precondition_value : rule.execution_preconditions) {
            resolved.push_back(preconditionFromReactionExecution(precondition_value));
        }
    }
    return Result<std::vector<PlanPrecondition>>::ok(dedupe(std::move(resolved)));
}

Result<std::vector<PlanPrecondition>> ReactionPlanningAdapter::preconditionsForEffect(const std::string& effect_key) const {
    return preconditionsForEffect(effect_key, pathfinder::reaction::fixtures::coreP28Rules());
}

} // namespace pathfinder::agent_reasoning
