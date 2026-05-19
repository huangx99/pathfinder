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

PlanPrecondition quantityPrecondition(const std::string& key, int required, bool can_plan) {
    return precondition("object.quantity", key, "gte." + std::to_string(required), can_plan);
}

PlanPrecondition statePrecondition(const std::string& object_key, const std::string& state_key, int required, bool can_plan) {
    return precondition("object.state", object_key + "." + state_key, "gte." + std::to_string(required), can_plan);
}

std::optional<PlanPrecondition> preconditionFromReactionPattern(const pathfinder::reaction::ReactionObjectPattern& pattern) {
    if (pattern.role == ReactionObjectRole::Source) {
        if (pattern.definition_id && pattern.definition_id->value() == "def_fire_source") return quantityPrecondition("camp_fire", 1, true);
        for (const auto& tag : pattern.required_tag_keys) {
            if (tag == "fire_source") return quantityPrecondition("camp_fire", 1, true);
        }
    }
    if (pattern.role == ReactionObjectRole::Material) {
        if (pattern.definition_id && pattern.definition_id->value() == "def_raw_wood") return quantityPrecondition("wood", 1, false);
        if (pattern.definition_id && pattern.definition_id->value() == "def_whetstone") return quantityPrecondition("whetstone", 1, false);
        for (const auto& tag : pattern.required_tag_keys) {
            if (tag == "combustible" || tag == "branch_like") return quantityPrecondition("wood_processed", 1, true);
            if (tag == "wood_material") return quantityPrecondition("wood", 1, false);
            if (tag == "sharpening_tool") return quantityPrecondition("whetstone", 1, false);
        }
    }
    if (pattern.role == ReactionObjectRole::Tool) {
        if (pattern.definition_id && pattern.definition_id->value() == "def_axe") return quantityPrecondition("axe", 1, false);
        for (const auto& tag : pattern.required_tag_keys) {
            if (tag == "cutting_tool") return quantityPrecondition("axe", 1, false);
        }
    }
    if (pattern.role == ReactionObjectRole::Target) {
        for (const auto& tag : pattern.required_tag_keys) {
            if (tag == "water_source") return quantityPrecondition("water", 1, false);
        }
    }
    return std::nullopt;
}

std::optional<PlanPrecondition> preconditionFromReactionCondition(const pathfinder::condition::ConditionExpressionRef& ref) {
    const auto& key = ref.inline_canonical_key;
    if (key == "condition:source_state:eq:sharp" || key == "condition:target_state:eq:sharp") return statePrecondition("axe", "sharpness", 1, true);
    if (key == "condition:source_state:eq:burning") return quantityPrecondition("camp_fire", 1, true);
    return std::nullopt;
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
            auto required = preconditionFromReactionPattern(pattern);
            if (required) resolved.push_back(*required);
        }
        for (const auto& condition_ref : rule.condition_refs) {
            auto required = preconditionFromReactionCondition(condition_ref);
            if (required) resolved.push_back(*required);
        }
    }
    return Result<std::vector<PlanPrecondition>>::ok(dedupe(std::move(resolved)));
}

Result<std::vector<PlanPrecondition>> ReactionPlanningAdapter::preconditionsForEffect(const std::string& effect_key) const {
    return preconditionsForEffect(effect_key, pathfinder::reaction::fixtures::coreP28Rules());
}

} // namespace pathfinder::agent_reasoning
