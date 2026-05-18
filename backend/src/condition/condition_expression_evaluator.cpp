#include "pathfinder/condition/condition_expression_evaluator.h"
#include "pathfinder/condition/condition_summary.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::makeError;

static bool isFiniteRatio(double value) {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

static double clampRatio(double value) {
    if (!std::isfinite(value)) return 0.0;
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

static bool containsValue(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

static std::vector<std::string> split(const std::string& value, char delim) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delim)) parts.push_back(part);
    return parts;
}

static std::string suffixAfterDelimiters(const std::string& value, char delim, int delimiter_count) {
    size_t pos = 0;
    for (int i = 0; i < delimiter_count; ++i) {
        pos = value.find(delim, pos);
        if (pos == std::string::npos) return "";
        ++pos;
    }
    return value.substr(pos);
}

static double numberValue(const ConditionExpressionValue& value) {
    if (auto v = std::get_if<int64_t>(&value.value)) return static_cast<double>(*v);
    if (auto v = std::get_if<double>(&value.value)) return *v;
    return 0.0;
}

static std::string stringValue(const ConditionExpressionValue& value) {
    if (auto v = std::get_if<std::string>(&value.value)) return *v;
    return value.stableDebugString();
}

static std::optional<double> numericField(const ConditionEvaluationContext& context, const std::string& field) {
    if (context.civilization) {
        auto it = context.civilization->numeric_fields.find(field);
        if (it != context.civilization->numeric_fields.end()) return it->second;
    }
    if (context.conflict) {
        auto it = context.conflict->numeric_fields.find(field);
        if (it != context.conflict->numeric_fields.end()) return it->second;
    }
    if (field == "tribe.cohesion" && context.tribe) return context.tribe->cohesion;
    if (field == "tribe.trust" && context.tribe) return context.tribe->trust;
    if (field == "actor.trust" && context.actor) return context.actor->trust;
    return std::nullopt;
}

static std::vector<std::string> stringSetField(const ConditionEvaluationContext& context, const std::string& field) {
    if (field == "target.state" && context.target) return context.target->state_keys;
    if (field == "target.tag" && context.target) return context.target->tag_keys;
    if (field == "source.state" && context.source) return context.source->state_keys;
    if (field == "source.tag" && context.source) return context.source->tag_keys;
    if (field == "actor.requirement" && context.actor) return context.actor->requirement_keys;
    if (field == "actor.capability" && context.actor) return context.actor->capability_keys;
    if (field == "actor.tag" && context.actor) return context.actor->tag_keys;
    if (field == "knowledge.claim" && context.knowledge) return context.knowledge->canonical_claim_keys;
    if (field == "knowledge.source" && context.knowledge) return context.knowledge->source_keys;
    if (field == "knowledge.condition" && context.knowledge) return context.knowledge->condition_keys;
    if (field == "civilization.capability" && context.civilization) return context.civilization->capability_keys;
    if (field == "tribe.capability" && context.tribe) return context.tribe->capability_keys;
    if (field == "conflict.summary" && context.conflict) return context.conflict->summary_keys;
    return {};
}

static ConditionEvaluationResult makeResult(bool matched, double score, const std::string& canonical_key, const std::string& summary_key, std::vector<std::string> steps) {
    ConditionEvaluationResult result;
    result.matched = matched;
    result.score = clampRatio(score);
    result.canonical_condition_key = canonical_key;
    result.summary_key = summary_key;
    result.trace.expression_key = canonical_key;
    result.trace.matched = matched;
    result.trace.score = result.score;
    result.trace.steps = std::move(steps);
    return result;
}

static Result<ConditionEvaluationResult> evalNode(const ConditionExpressionNode& node, const ConditionEvaluationContext& context) {
    auto valid = node.validateBasic();
    if (valid.is_error()) return Result<ConditionEvaluationResult>::fail(valid.errors());

    if (node.kind == ConditionExpressionNodeKind::LiteralBool) {
        bool matched = false;
        if (auto v = std::get_if<bool>(&node.value.value)) matched = *v;
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, "inline:literal_bool", node.summary_key, {"literal_bool"}));
    }
    if (node.kind == ConditionExpressionNodeKind::Compare) {
        const auto expected = numberValue(node.value);
        auto actual_opt = numericField(context, node.field_path);
        if (!actual_opt) {
            return Result<ConditionEvaluationResult>::ok(makeResult(false, 0.0, "inline:compare:" + node.field_path, node.summary_key, {"missing_field:" + node.field_path}));
        }
        const double actual = *actual_opt;
        bool matched = false;
        double score = 0.0;
        if (node.op == ConditionExpressionOperator::Eq) {
            matched = actual == expected;
            score = matched ? 1.0 : 0.0;
        } else if (node.op == ConditionExpressionOperator::NotEq) {
            matched = actual != expected;
            score = matched ? 1.0 : 0.0;
        } else if (node.op == ConditionExpressionOperator::Gt) {
            matched = actual > expected;
            score = expected <= 0.0 ? (matched ? 1.0 : 0.0) : clampRatio(actual / expected);
        } else if (node.op == ConditionExpressionOperator::Gte) {
            matched = actual >= expected;
            score = expected <= 0.0 ? (matched ? 1.0 : 0.0) : clampRatio(actual / expected);
        } else if (node.op == ConditionExpressionOperator::Lt) {
            matched = actual < expected;
            score = matched ? 1.0 : clampRatio(expected <= 0.0 ? 0.0 : expected / std::max(actual, 0.0001));
        } else if (node.op == ConditionExpressionOperator::Lte) {
            matched = actual <= expected;
            score = matched ? 1.0 : clampRatio(expected <= 0.0 ? 0.0 : expected / std::max(actual, 0.0001));
        }
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, score, "inline:compare:" + node.field_path, node.summary_key, {"compare:" + node.field_path}));
    }
    if (node.kind == ConditionExpressionNodeKind::Contains || node.kind == ConditionExpressionNodeKind::HasState || node.kind == ConditionExpressionNodeKind::HasTag || node.kind == ConditionExpressionNodeKind::HasCapability || node.kind == ConditionExpressionNodeKind::KnowledgeHasClaim) {
        const auto values = stringSetField(context, node.field_path);
        const auto expected = stringValue(node.value);
        const bool matched = containsValue(values, expected);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, "inline:contains:" + node.field_path + ":" + expected, node.summary_key, {"contains:" + node.field_path + ":" + expected}));
    }
    if (node.kind == ConditionExpressionNodeKind::And) {
        bool matched = true;
        double score = 1.0;
        std::vector<std::string> steps{"and"};
        for (const auto& child : node.children) {
            auto child_result = evalNode(child, context);
            if (child_result.is_error()) return child_result;
            matched = matched && child_result.value().matched;
            score = std::min(score, child_result.value().score);
            steps.insert(steps.end(), child_result.value().trace.steps.begin(), child_result.value().trace.steps.end());
        }
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, score, "inline:and", node.summary_key, std::move(steps)));
    }
    if (node.kind == ConditionExpressionNodeKind::Or) {
        bool matched = false;
        double score = 0.0;
        std::vector<std::string> steps{"or"};
        for (const auto& child : node.children) {
            auto child_result = evalNode(child, context);
            if (child_result.is_error()) return child_result;
            matched = matched || child_result.value().matched;
            score = std::max(score, child_result.value().score);
            steps.insert(steps.end(), child_result.value().trace.steps.begin(), child_result.value().trace.steps.end());
        }
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, score, "inline:or", node.summary_key, std::move(steps)));
    }
    if (node.kind == ConditionExpressionNodeKind::Not) {
        auto child_result = evalNode(node.children.front(), context);
        if (child_result.is_error()) return child_result;
        const bool matched = !child_result.value().matched;
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, "inline:not", node.summary_key, {"not"}));
    }

    return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "unsupported condition node"));
}

static Result<ConditionEvaluationResult> evalCanonical(const std::string& canonical_key, const ConditionEvaluationContext& context) {
    if (containsConditionForbiddenKey(canonical_key)) {
        return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "canonical condition key forbidden"));
    }
    const auto parts = split(canonical_key, ':');
    if (parts.size() < 4 || parts[0] != "condition") {
        return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "invalid canonical condition key"));
    }

    const std::string domain = parts[1];
    std::string field;
    std::string op;
    std::string value;
    if (domain == "object_state" || domain == "actor_requirement" || domain == "knowledge_source" ||
        domain == "knowledge_condition" || domain == "test") {
        op = parts[2];
        value = suffixAfterDelimiters(canonical_key, ':', 3);
    } else {
        if (parts.size() < 5) {
            return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "invalid canonical condition key"));
        }
        field = parts[2];
        op = parts[3];
        value = suffixAfterDelimiters(canonical_key, ':', 4);
    }
    ConditionSummaryBuilder summary_builder;
    auto summary_result = summary_builder.summarizeCanonicalKey(canonical_key);
    const std::string summary_key = summary_result.is_ok() ? summary_result.value().summary_key : "condition.summary.unknown";

    if (domain == "and" && field == "all" && op == "eq") {
        const auto child_keys = split(value, '+');
        if (child_keys.empty()) return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "combined condition empty"));
        bool matched = true;
        double score = 1.0;
        std::vector<std::string> steps{"and:all"};
        for (const auto& child_key : child_keys) {
            auto child = evalCanonical(child_key, context);
            if (child.is_error()) return child;
            matched = matched && child.value().matched;
            score = std::min(score, child.value().score);
            steps.insert(steps.end(), child.value().trace.steps.begin(), child.value().trace.steps.end());
        }
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, score, canonical_key, summary_key, std::move(steps)));
    }
    if (domain == "object_state") {
        const bool matched = context.target && containsValue(context.target->state_keys, value);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, canonical_key, summary_key, {"object_state:" + value}));
    }
    if (domain == "actor_requirement") {
        const bool matched = context.actor && containsValue(context.actor->requirement_keys, value);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, canonical_key, summary_key, {"actor_requirement:" + value}));
    }
    if (domain == "knowledge_source") {
        const bool matched = context.knowledge && containsValue(context.knowledge->source_keys, value);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, canonical_key, summary_key, {"knowledge_source:" + value}));
    }
    if (domain == "knowledge_condition") {
        const bool matched = context.knowledge && containsValue(context.knowledge->condition_keys, value);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, canonical_key, summary_key, {"knowledge_condition:" + value}));
    }
    if (domain == "test") {
        const bool matched = containsValue(context.safe_context_keys, value);
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, matched ? 1.0 : 0.0, canonical_key, summary_key, {"test_condition:" + value}));
    }
    if (domain == "civilization") {
        if (!context.civilization) return Result<ConditionEvaluationResult>::ok(makeResult(false, 0.0, canonical_key, summary_key, {"missing_civilization_context"}));
        auto it = context.civilization->numeric_fields.find(field);
        if (it == context.civilization->numeric_fields.end()) return Result<ConditionEvaluationResult>::ok(makeResult(false, 0.0, canonical_key, summary_key, {"missing_civilization_field:" + field}));
        const double actual = it->second;
        double expected = 0.0;
        try {
            expected = std::stod(value);
        } catch (...) {
            return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "invalid civilization condition value"));
        }
        bool matched = false;
        double score = 0.0;
        if (op == "gte") {
            matched = actual >= expected;
            score = expected <= 0.0 ? (matched ? 1.0 : 0.0) : clampRatio(actual / expected);
        } else if (op == "lte") {
            matched = actual <= expected;
            score = matched ? 1.0 : clampRatio(expected <= 0.0 ? 0.0 : expected / std::max(actual, 0.0001));
        } else if (op == "eq") {
            matched = actual == expected;
            score = matched ? 1.0 : 0.0;
        } else {
            return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "unsupported civilization condition op"));
        }
        return Result<ConditionEvaluationResult>::ok(makeResult(matched, score, canonical_key, summary_key, {"civilization:" + field + ":" + op + ":" + value}));
    }

    return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "unsupported canonical condition domain"));
}

Result<void> ConditionEvaluationTrace::validateBasic() const {
    if (containsConditionForbiddenKey(expression_key) || containsConditionForbiddenKey(steps) || containsConditionForbiddenKey(reason_keys)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionEvaluationTrace contains forbidden key"));
    }
    if (!isFiniteRatio(score)) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ConditionEvaluationTrace score must be 0..1"));
    return Result<void>::ok();
}

Result<void> ConditionEvaluationResult::validateBasic() const {
    if (canonical_condition_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionEvaluationResult canonical_condition_key empty"));
    if (containsConditionForbiddenKey(canonical_condition_key) || containsConditionForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionEvaluationResult contains forbidden key"));
    }
    if (!isFiniteRatio(score)) return Result<void>::fail(makeError(ErrorCode::validation_value_out_of_range, "ConditionEvaluationResult score must be 0..1"));
    return trace.validateBasic();
}

Result<ConditionEvaluationResult> ConditionExpressionEvaluator::evaluate(const ConditionExpressionRef& ref, const ConditionEvaluationContext& context, const ConditionExpressionRegistry* registry) const {
    auto ref_valid = ref.validateBasic();
    if (ref_valid.is_error()) return Result<ConditionEvaluationResult>::fail(ref_valid.errors());
    auto context_valid = context.validateBasic();
    if (context_valid.is_error()) return Result<ConditionEvaluationResult>::fail(context_valid.errors());

    if (!ref.inline_canonical_key.empty()) {
        return evalCanonical(ref.inline_canonical_key, context);
    }
    if (registry) {
        const auto* definition = registry->findById(ref.expression_id);
        if (!definition) return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "condition expression id not found"));
        return evaluateDefinition(*definition, context);
    }
    return Result<ConditionEvaluationResult>::fail(makeError(ErrorCode::validation_failed, "condition registry required"));
}

Result<ConditionEvaluationResult> ConditionExpressionEvaluator::evaluateDefinition(const ConditionExpressionDefinition& definition, const ConditionEvaluationContext& context) const {
    auto def_valid = definition.validateBasic();
    if (def_valid.is_error()) return Result<ConditionEvaluationResult>::fail(def_valid.errors());
    auto context_valid = context.validateBasic();
    if (context_valid.is_error()) return Result<ConditionEvaluationResult>::fail(context_valid.errors());

    auto result = evalNode(definition.root, context);
    if (result.is_error()) return result;
    auto value = result.value();
    value.canonical_condition_key = definition.canonical_key;
    value.summary_key = definition.summary_key;
    value.trace.expression_key = definition.canonical_key;
    auto valid = value.validateBasic();
    if (valid.is_error()) return Result<ConditionEvaluationResult>::fail(valid.errors());
    return Result<ConditionEvaluationResult>::ok(std::move(value));
}

} // namespace pathfinder::condition
