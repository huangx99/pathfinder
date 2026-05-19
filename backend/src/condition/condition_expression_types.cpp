#include "pathfinder/condition/condition_expression_types.h"
#include "pathfinder/foundation/error.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace pathfinder::condition {

using pathfinder::foundation::ErrorCode;
using pathfinder::foundation::Result;
using pathfinder::foundation::isValidIdString;
using pathfinder::foundation::makeError;

static std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool containsConditionForbiddenKey(const std::string& key) {
    const auto lower = lowerCopy(key);
    static const std::vector<std::string> forbidden = {
        "hidden_truth", "true_trait", "real_effect", "raw_state", "gamestate", "game_state",
        "savegame", "save_game", "raw_damage", "actual_damage", "true_hp", "actual_hp",
        "hp_delta", "death", "kill", "loot", "corpse", "unlocked=true", "frontend_unlock",
        "civilization_level", "random", "system_clock", "mutation", "direct_state"
    };
    for (const auto& token : forbidden) {
        if (lower.find(token) != std::string::npos) return true;
    }
    return false;
}

bool containsConditionForbiddenKey(const std::vector<std::string>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](const auto& key) { return containsConditionForbiddenKey(key); });
}

static bool containsForbiddenValue(const ConditionExpressionValue& value) {
    if (auto text = std::get_if<std::string>(&value.value)) return containsConditionForbiddenKey(*text);
    if (auto values = std::get_if<std::vector<std::string>>(&value.value)) return containsConditionForbiddenKey(*values);
    return false;
}

bool ConditionExpressionValue::isEmpty() const {
    return std::holds_alternative<std::monostate>(value);
}

std::string ConditionExpressionValue::stableDebugString() const {
    if (std::holds_alternative<std::monostate>(value)) return "<empty>";
    if (auto v = std::get_if<bool>(&value)) return *v ? "true" : "false";
    if (auto v = std::get_if<int64_t>(&value)) return std::to_string(*v);
    if (auto v = std::get_if<double>(&value)) {
        std::ostringstream out;
        out << *v;
        return out.str();
    }
    if (auto v = std::get_if<std::string>(&value)) return *v;
    const auto& values = std::get<std::vector<std::string>>(value);
    std::vector<std::string> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    std::string result = "[";
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) result += ",";
        result += sorted[i];
    }
    result += "]";
    return result;
}

std::string toString(ConditionExpressionNodeKind kind) {
    switch (kind) {
        case ConditionExpressionNodeKind::Unknown: return "unknown";
        case ConditionExpressionNodeKind::LiteralBool: return "literal_bool";
        case ConditionExpressionNodeKind::LiteralNumber: return "literal_number";
        case ConditionExpressionNodeKind::LiteralString: return "literal_string";
        case ConditionExpressionNodeKind::FieldRef: return "field_ref";
        case ConditionExpressionNodeKind::Compare: return "compare";
        case ConditionExpressionNodeKind::Contains: return "contains";
        case ConditionExpressionNodeKind::HasTag: return "has_tag";
        case ConditionExpressionNodeKind::HasState: return "has_state";
        case ConditionExpressionNodeKind::HasCapability: return "has_capability";
        case ConditionExpressionNodeKind::KnowledgeHasClaim: return "knowledge_has_claim";
        case ConditionExpressionNodeKind::And: return "and";
        case ConditionExpressionNodeKind::Or: return "or";
        case ConditionExpressionNodeKind::Not: return "not";
        case ConditionExpressionNodeKind::TestOnly: return "test_only";
    }
    return "unknown";
}

std::string toString(ConditionExpressionOperator op) {
    switch (op) {
        case ConditionExpressionOperator::Unknown: return "unknown";
        case ConditionExpressionOperator::Eq: return "eq";
        case ConditionExpressionOperator::NotEq: return "neq";
        case ConditionExpressionOperator::Gt: return "gt";
        case ConditionExpressionOperator::Gte: return "gte";
        case ConditionExpressionOperator::Lt: return "lt";
        case ConditionExpressionOperator::Lte: return "lte";
        case ConditionExpressionOperator::In: return "in";
        case ConditionExpressionOperator::Contains: return "contains";
        case ConditionExpressionOperator::HasTag: return "has_tag";
        case ConditionExpressionOperator::HasState: return "has_state";
        case ConditionExpressionOperator::HasCapability: return "has_capability";
        case ConditionExpressionOperator::TestOnly: return "test_only";
    }
    return "unknown";
}

Result<ConditionExpressionOperator> conditionOperatorFromString(const std::string& value) {
    if (value == "eq") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Eq);
    if (value == "neq") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::NotEq);
    if (value == "gt") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Gt);
    if (value == "gte") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Gte);
    if (value == "lt") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Lt);
    if (value == "lte") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Lte);
    if (value == "in") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::In);
    if (value == "contains") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::Contains);
    if (value == "has_tag") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::HasTag);
    if (value == "has_state") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::HasState);
    if (value == "has_capability") return Result<ConditionExpressionOperator>::ok(ConditionExpressionOperator::HasCapability);
    return Result<ConditionExpressionOperator>::fail(makeError(ErrorCode::validation_enum_unknown, "unknown condition operator: " + value));
}

static bool validNodeKind(ConditionExpressionNodeKind kind) {
    return kind != ConditionExpressionNodeKind::Unknown && kind != ConditionExpressionNodeKind::TestOnly;
}

static bool validOperator(ConditionExpressionOperator op) {
    return op != ConditionExpressionOperator::Unknown && op != ConditionExpressionOperator::TestOnly;
}

Result<void> ConditionExpressionNode::validateBasic() const {
    if (!validNodeKind(kind)) return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConditionExpressionNode kind invalid"));
    if (containsConditionForbiddenKey(field_path) || containsConditionForbiddenKey(summary_key) || containsForbiddenValue(value)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionNode contains forbidden key"));
    }
    if ((kind == ConditionExpressionNodeKind::Compare || kind == ConditionExpressionNodeKind::Contains ||
         kind == ConditionExpressionNodeKind::HasState || kind == ConditionExpressionNodeKind::HasTag ||
         kind == ConditionExpressionNodeKind::HasCapability || kind == ConditionExpressionNodeKind::KnowledgeHasClaim) &&
        field_path.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionNode field_path empty"));
    }
    if (kind == ConditionExpressionNodeKind::Compare && !validOperator(op)) {
        return Result<void>::fail(makeError(ErrorCode::validation_enum_unknown, "ConditionExpressionNode operator invalid"));
    }
    if ((kind == ConditionExpressionNodeKind::And || kind == ConditionExpressionNodeKind::Or) && children.empty()) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionNode composite children empty"));
    }
    if (kind == ConditionExpressionNodeKind::Not && children.size() != 1) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionNode not requires one child"));
    }
    for (const auto& child : children) {
        auto result = child.validateBasic();
        if (result.is_error()) return result;
    }
    return Result<void>::ok();
}

bool ConditionExpressionRef::empty() const {
    return expression_id.empty() && inline_canonical_key.empty();
}

Result<void> ConditionExpressionRef::validateBasic() const {
    if (empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionRef empty"));
    if (!expression_id.empty() && !isValidIdString(expression_id.value())) {
        return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ConditionExpressionRef expression_id invalid"));
    }
    if (containsConditionForbiddenKey(inline_canonical_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionRef inline_canonical_key forbidden"));
    }
    return Result<void>::ok();
}

Result<void> ConditionExpressionDefinition::validateBasic() const {
    if (expression_id.empty()) return Result<void>::fail(makeError(ErrorCode::id_missing, "ConditionExpressionDefinition expression_id missing"));
    if (!isValidIdString(expression_id.value())) return Result<void>::fail(makeError(ErrorCode::id_invalid_format, "ConditionExpressionDefinition expression_id invalid"));
    if (context_type.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionDefinition context_type empty"));
    if (canonical_key.empty()) return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionDefinition canonical_key empty"));
    if (containsConditionForbiddenKey(context_type) || containsConditionForbiddenKey(canonical_key) || containsConditionForbiddenKey(summary_key)) {
        return Result<void>::fail(makeError(ErrorCode::validation_failed, "ConditionExpressionDefinition contains forbidden key"));
    }
    return root.validateBasic();
}

} // namespace pathfinder::condition
